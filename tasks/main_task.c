#include <io.h>
#include <string.h>
#include <syscall.h>

#include <uart_server.h>
#include <name_server.h>
#include <clock.h>
#include <sensor.h>
#include <switch.h>
#include <train_command.h>
#include <train_controller.h>
#include <train_profile.h>
#include <track_server.h>
#include <term.h>

#include <location.h>
#include <cab_service.h>

/*
 * main_task: receive loop; receive sensors, receive input commands, process input cmds, print sensors
 * sensors_task: send command to read sensors, read sensors, send updated to main
 * cli_task: receive characters, echos character back to term_server. on full command, sends to main
 *
 * train_server: receives commands, writes commands to train
 * term_server: receives string commands, writes string to terminal
 */

void train_controller_entry();
void cli_task_entry();
void cab_service_task();

static int uart_tid, clock_tid;
static int train_command_tid, term_tid;
static int cli_tid, time_tid;
static int train_controller_tid, switch_tid;
static int track_server_tid;
static int cab_service_tid;

void term_switch_update(int sw, int direction) {
	/*
	char buf[100], *p = buf;
	p += sprintf(p, CURSOR_SAVE);

	char dirSymbol = direction == SWITCH_CURVED ? 'C' : 'S';

	if (sw >= 1 && sw <= 11) {
		p += sprintf(p, CURSOR_POS, TERM_SWITCH_TOP_OFFSET + sw,
				TERM_SWITCH_LEFT_OFFSET);
		p += sprintf(p, "%d = %c ", sw, dirSymbol);
	} else if (sw >= 12 && sw <= 18) {
		p += sprintf(p, CURSOR_POS, TERM_SWITCH_TOP_OFFSET + sw - 11,
				TERM_SWITCH_LEFT_OFFSET + 10);
		p += sprintf(p, "%d = %c ", sw, dirSymbol);
	} else if (sw >= 0x9A && sw <= 0x9C) {
		p += sprintf(p, CURSOR_POS, TERM_SWITCH_TOP_OFFSET + 8 + (sw - 0x9A),
				TERM_SWITCH_LEFT_OFFSET + 10);
		p += sprintf(p, "%d = %c ", sw, dirSymbol);
	} else if (sw == 0x99) {
		p += sprintf(p, CURSOR_POS, TERM_SWITCH_TOP_OFFSET + 11,
				TERM_SWITCH_LEFT_OFFSET + 10);
		p += sprintf(p, "%d = %c ", sw, dirSymbol);
	}

	p += sprintf(p, CURSOR_RESTORE);

	// write to term
	TermPrint(buf, term_tid);*/
}

// returns 1 to quit the main_task loop. 0 otherwise.
static int process_cmd(char *msg) {

	// parse first word
	char word[100];
	char *p = msg;
	p += parse_token(p, word);

	if (!strcmp(word, "tr")) {

		char train_s[8], speed_s[8];
		int train, speed;
		p += parse_token(p, train_s); // read in the train string
		p += parse_token(p, speed_s); // read in the speed string

		train = a2i(train_s);
		speed = a2i(speed_s);

        if (speed < 0 || speed > 14) {
        	TermPrint("That is not a valid speed. Must be between 0-14, inclusive.\r", term_tid);
        } else {
        	TrainControllerSpeed(train, speed, train_controller_tid);
        }
	} else if (!strcmp(word, "rv")) {

		char train_s[8];
		int train;

		p += parse_token(p, train_s);
		train = a2i(train_s);

        TrainControllerReverse(train, train_controller_tid);

	} else if (!strcmp(word, "sw")) {

		char sw_s[8], direction[4];
		int sw;

		p += parse_token(p, sw_s);
		sw = a2i(sw_s);

		p += parse_token(p, direction);

		if (IS_SWITCH(sw)) {
			if (direction[0] == 'C' || direction[0] == 'c') {
				SwitchSetDirection(sw, SWITCH_CURVED, switch_tid);
			} else if (direction[0] == 'S' || direction[0] == 's') {
				SwitchSetDirection(sw, SWITCH_STRAIGHT, switch_tid);
			} else {
				TermPrint("That is not a valid switch direction!", term_tid);
			}
		} else {
			TermPrint("That is not a valid switch!\r", term_tid);
		}
	} else if (!strcmp(word, "route")) {
		// route <train> <sensor>
		// eg. route 38 A4

		char train_s[8];
		char sens_s[8];
		int train, module, sensor;

		p += parse_token(p, train_s);
		train = a2i(train_s);

		p += parse_token(p, sens_s);
		module = (*sens_s) - 'A';
		sensor = a2i(sens_s + 1);

		if (sensor < 1 || sensor > 16 || module < 0 || module >= 5) {
			TermPrint("That is not a valid sensor!\r", term_tid);
		} else if (train != 0 && sensor >= 1 && sensor <= 16 && module >= 0	&& module < 5) {
			TrainControllerRoute(train, SENSOR_TO_NODE(module, sensor - 1), 0, train_controller_tid);
		}
	} else if (!strcmp(word, "cab")) {
		char sens_s[8];
		p += parse_token(p, sens_s);

		int pickup_module = (*sens_s) - 'A';
		int pickup_sensor = a2i(sens_s + 1);

		if (!(pickup_sensor >= 1 && pickup_sensor <= 16 && pickup_module >= 0
				&& pickup_module < 5)) {
			TermPrint("Invalid pickup sensor\r", term_tid);
			return 0;
		}

		p += parse_token(p, sens_s);
		int dropoff_module = (*sens_s) - 'A';
		int dropoff_sensor = a2i(sens_s + 1);

		if (!(dropoff_sensor >= 1 && dropoff_sensor <= 16 && dropoff_module >= 0
				&& dropoff_module < 5)) {
			TermPrint("Invalid dropoff sensor\r", term_tid);
			return 0;
		}

		struct location pickup;
		pickup.node = SENSOR_TO_NODE(pickup_module, pickup_sensor - 1);
		pickup.offset = 0;

		struct location dropoff;
		dropoff.node = SENSOR_TO_NODE(dropoff_module, dropoff_sensor - 1);
		dropoff.offset = 0;

		CabRequest(&pickup, &dropoff, cab_service_tid);
	} else if (!strcmp(word, "q")) {
		TrainCommandStop(train_command_tid);
		Delay(10, clock_tid); // wait 100ms for the command to flush out by the uart server
		Shutdown();
		return 1;
	}

	/**** PROJECT STUFF ****/
	/*
	 * taxi service:
	 *
	 * 	maintain list of free trains
	 * 	on_free_cab:
	 * 		add cab to free cab list
	 * 		if cab request exists, dispatch top cab
	 * 	on_dispatch:
	 * 		add cab to request list
	 * 		if free cab exists, dispatch cab
	 *
	 */

	return 0;
}

static void time_task_entry() {
	RegisterAs("time_task");

	while (1) {
		Delay(9, clock_tid);
		int ticks = Time(clock_tid);
		int secs = ticks / 100;
		int msecs = (ticks % 100);

		char buf[100], *p = buf;
		p += sprintf(p, CURSOR_SAVE CURSOR_POS, 0, 0);
		p += sprintf(p, "Time: %d.%d   ", secs, msecs);
		p += sprintf(p, CURSOR_RESTORE);
		TermPrint(buf, term_tid);
	}

	Exit();
}

static void position_printer_entry() {
	int clock_tid = WhoIs(CLOCK_SERVER);
	int term_tid = WhoIs(TERM_SERVER);
	int track_tid = WhoIs(TRACK_SERVER);
	int train_controller_tid = WhoIs(TRAIN_CONTROLLER);

	while (1) {
		char module = ' ';
		int sensor = 0;
		int offset = 0;

		struct train_position position41 = TrainControllerPosition(41, train_controller_tid);
		struct train_position position35 = TrainControllerPosition(35, train_controller_tid);
		struct train_position position36 = TrainControllerPosition(36, train_controller_tid);

		char reservation35[300];
		char reservation41[300];
		char reservation36[300];
		int reservation35_len = TrackReservationString(35, reservation35, sizeof(reservation35), track_tid);
		int reservation41_len = TrackReservationString(41, reservation41, sizeof(reservation41), track_tid);
		int reservation36_len = TrackReservationString(36, reservation36, sizeof(reservation36), track_tid);

		if (position41.sensor != -1) {
			module = (position41.sensor / NUM_SENSORS_PER_MODULE) + 'A';
			sensor = (position41.sensor % NUM_SENSORS_PER_MODULE) + 1;
			offset = position41.offset;
			char msg[300], *p = msg;
			p += sprintf(p,
			        CURSOR_SAVE
			        CURSOR_POS "Train 41: %c%d %d mm  @  %d mm/s %s %s   "
			        "(Est./Actual) = (%d/%d)    ",
			        TERM_SENSOR_TOP_OFFSET - 2,
			        TERM_SENSOR_LEFT_OFFSET + 30,
			        module, sensor, offset, position41.velocity,
			        (position41.action == TYPE_ACCELERATING ? "ACCEL" : "CONST"),
			        (position41.direction == DIRECTION_REVERSED ? "[R]  " : "     "),
			        position41.estimated_time, position41.actual_time);
			if (position41.destination != -1) {
				char mod = (position41.destination / NUM_SENSORS_PER_MODULE) + 'A';
				int sen = (position41.destination % NUM_SENSORS_PER_MODULE) + 1;

				p += sprintf(p, "HEADING TO %c%d  ", mod, sen);
			} else {
				p += sprintf(p, "                 ");
			}

			p += sprintf(p, CURSOR_RESTORE);

			TermPrint(msg, term_tid);
		}

		if (reservation41_len > 0) {
			char msg[300];
			char *p = msg;

			p += sprintf(p, CURSOR_SAVE CURSOR_POS CURSOR_CLEAR_LINE_AFTER, TERM_RESERVATION_LINE + 1, 0);
			p += sprintf(p, "%d: %s", 41, reservation41);
			p += sprintf(p, CURSOR_RESTORE);

			TermPrint(msg, term_tid);
		}

		if (position35.sensor != -1) {
			module = (position35.sensor / NUM_SENSORS_PER_MODULE) + 'A';
			sensor = (position35.sensor % NUM_SENSORS_PER_MODULE) + 1;
			offset = position35.offset;
			char msg[300], *p = msg;
			p += sprintf(
					p,
					CURSOR_SAVE
					CURSOR_POS "Train 35: %c%d %d mm  @  %d mm/s %s   "
					"(Est./Actual) = (%d/%d)    ",
					TERM_SENSOR_TOP_OFFSET - 1,
					TERM_SENSOR_LEFT_OFFSET + 30,
					module,
					sensor,
					offset,
					position35.velocity,
					(position35.direction == DIRECTION_REVERSED ?
							"[R]  " : "     "), position35.estimated_time,
					position35.actual_time);
			if (position35.destination != -1) {
				char mod = (position35.destination / NUM_SENSORS_PER_MODULE)
						+ 'A';
				int sen = (position35.destination % NUM_SENSORS_PER_MODULE) + 1;
				p += sprintf(p, "HEADING TO %c%d  ", mod, sen);
			} else {
				p += sprintf(p, "                 ");
			}

			p += sprintf(p, CURSOR_RESTORE);

			TermPrint(msg, term_tid);
		}

		if (reservation35_len > 0) {
			char msg[300];
			char *p = msg;

			p += sprintf(p, CURSOR_SAVE CURSOR_POS CURSOR_CLEAR_LINE_AFTER,
					TERM_RESERVATION_LINE, 0);
			p += sprintf(p, "%d: %s", 35, reservation35);
			p += sprintf(p, CURSOR_RESTORE);

			TermPrint(msg, term_tid);
		}

//		if (position36.sensor != -1) {
//			module = (position36.sensor / NUM_SENSORS_PER_MODULE) + 'A';
//			sensor = (position36.sensor % NUM_SENSORS_PER_MODULE) + 1;
//			offset = position36.offset;
//			char msg[300], *p = msg;
//			p += sprintf(
//					p,
//					CURSOR_SAVE
//					CURSOR_POS "Train 36: %c%d %d mm  @  %d mm/s %s   "
//					"(Est./Actual) = (%d/%d)    ",
//					TERM_SENSOR_TOP_OFFSET,
//					TERM_SENSOR_LEFT_OFFSET + 30,
//					module,
//					sensor,
//					offset,
//					position36.velocity,
//					(position36.direction == DIRECTION_REVERSED ?
//							"[R]  " : "     "), position36.estimated_time,
//					position36.actual_time);
//			if (position36.destination != -1) {
//				char mod = (position36.destination / NUM_SENSORS_PER_MODULE)
//						+ 'A';
//				int sen = (position36.destination % NUM_SENSORS_PER_MODULE) + 1;
//				p += sprintf(p, "HEADING TO %c%d  ", mod, sen);
//			} else {
//				p += sprintf(p, "                 ");
//			}
//
//			p += sprintf(p, CURSOR_RESTORE);
//
//			TermPrint(msg, term_tid);
//		}
//
//		if (reservation36_len > 0) {
//			char msg[300];
//			char *p = msg;
//
//			p += sprintf(p, CURSOR_SAVE CURSOR_POS CURSOR_CLEAR_LINE_AFTER,
//					TERM_RESERVATION_LINE, 0);
//			p += sprintf(p, "%d: %s", 36, reservation36);
//			p += sprintf(p, CURSOR_RESTORE);
//
//			TermPrint(msg, term_tid);
//		}

		Delay(10, clock_tid);
	}
}

void main_task_entry() {
	RegisterAs("main_task");

	/* ASSUMES main_task_entry is running at PRIORITY_NORMAL */
	uart_tid = WhoIs(UART_SERVER);
	clock_tid = WhoIs(CLOCK_SERVER);
	train_command_tid = WhoIs(TRAIN_COMMAND);
	switch_tid = WhoIs(SWITCH_SERVER);
	term_tid = WhoIs(TERM_SERVER);
	track_server_tid = WhoIs(TRACK_SERVER);

	/* make helper tasks */
	cli_tid = Create(PRIORITY_NORMAL, &cli_task_entry);
	time_tid = Create(PRIORITY_NORMAL - 1, &time_task_entry);
	train_controller_tid = Create(PRIORITY_NORMAL, &train_controller_entry);
	cab_service_tid = Create(PRIORITY_NORMAL - 1, &cab_service_task);

	/* init train stuff */
	TrainCommandGo(train_command_tid);

	{
		char buf[300], *p = buf;
		p += sprintf(p, CURSOR_CLEAR_SCREEN);
		p += sprintf(p, CURSOR_SCROLL, 0, 999);
		p += sprintf(p, CURSOR_POS "SENSORS: ", TERM_SENSOR_TOP_OFFSET - 1, TERM_SENSOR_LEFT_OFFSET);
		p += sprintf(p, CURSOR_POS "COMMAND LINE: ", TERM_PROMPT_TOP - 1, 3);
		p += sprintf(p, CURSOR_SCROLL, TERM_PROMPT_TOP, TERM_PROMPT_BOTTOM);
		p += sprintf(p, CURSOR_POS, 999, 0);

		TermPrint(buf, term_tid);
	}

	int i;
	for (i = 1; i <= 18; i++) {
		term_switch_update(i, SWITCH_CURVED);
	}

	term_switch_update(0x9A, SWITCH_CURVED);
	term_switch_update(0x9B, SWITCH_CURVED);
	term_switch_update(0x9C, SWITCH_CURVED);
	term_switch_update(0x99, SWITCH_CURVED);

	term_switch_update(0x9C, SWITCH_STRAIGHT); // 156
	term_switch_update(0x99, SWITCH_STRAIGHT); // 153
	term_switch_update(17, SWITCH_STRAIGHT);
	term_switch_update(10, SWITCH_STRAIGHT);

	char msg_buffer[128];
	int sending_tid;

	Create(PRIORITY_NORMAL - 1, &position_printer_entry);
	int sensor_courier_tid = CreateSensorCourier(PRIORITY_HIGHEST - 1);
	int switch_courier_tid = CreateSwitchCourier(PRIORITY_HIGHEST - 1);

	int quit = 0;
	int ok = 1;
	struct track_node *track = get_track();
	while (quit == 0) {
		Receive(&sending_tid, msg_buffer, sizeof(msg_buffer));

		if (sending_tid == sensor_courier_tid) {
			struct sensor_report* msg = (struct sensor_report*) msg_buffer;

			int sensor_i;
			for (sensor_i = 0; sensor_i < msg->num_sensors; sensor_i++) {
				char buf[300], *p = buf;
				p += sprintf(p, CURSOR_SAVE CURSOR_SCROLL, TERM_SENSOR_TOP_OFFSET,
						TERM_SENSOR_BOTTOM_OFFSET);
				p += sprintf(p, CURSOR_POS, TERM_SENSOR_BOTTOM_OFFSET,
						TERM_SENSOR_LEFT_OFFSET);
				p += sprintf(p, "%s\r", track[(int)msg->sensor_nodes[sensor_i]].name);
				p += sprintf(p, CURSOR_SCROLL CURSOR_RESTORE, TERM_PROMPT_TOP,
						TERM_PROMPT_BOTTOM);

				TermPrint(buf, term_tid);
			}

		} else if (sending_tid == switch_courier_tid) {
			struct switch_report* msg = (struct switch_report*) msg_buffer;
			term_switch_update(msg->sw, msg->direction);
		} else if (sending_tid == cli_tid) {
			quit = process_cmd(msg_buffer);
		} else {
			AssertF(0, "Invalid command to main_task.");
		}

		Reply(sending_tid, (char*) &ok, sizeof(ok));
	}

	Printf("Quitting!\r");

	Exit();
}

