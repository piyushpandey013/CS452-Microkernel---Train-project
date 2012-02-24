#include <sensor.h>
#include <switch.h>
#include <term.h>
#include <train.h>
#include <clock.h>
#include <name_server.h>
#include <train_command.h>
#include <io.h>

#include <track_data.h>
#include <track_node.h>
#include <string.h>
#include <uart_server.h>

static int TRAIN;

static struct track_node track[TRACK_MAX];

void stopping_distance_task() {

	TRAIN = 21;

//	int term_tid = WhoIs("term_tid");
	int train_tid = WhoIs(TRAIN_COMMAND);
	int uart_tid = WhoIs(UART_SERVER);
	int sensor_tid = WhoIs(SENSOR_SERVER);
	int clock_tid = WhoIs(CLOCK_SERVER);

	init_tracka(track);

	Delay(100, clock_tid);
	TrainCommandSwitchStraight(8, train_tid);
	TrainCommandSwitchStraight(6, train_tid);
	TrainCommandSwitchStraight(9, train_tid);
	TrainCommandSwitchStraight(7, train_tid);
	TrainCommandSwitchStraight(15, train_tid);
	TrainCommandSwitchStraight(11, train_tid);
	TrainCommandSwitchStraight(12, train_tid);

	Printf("press 'a' for train 21, 'b' for 38\r");
	if (Getc(COM2, uart_tid) == 'a') TRAIN = 21;
	else TRAIN = 38;

	TrainCommandSpeed(TRAIN, 0, train_tid);

	int speed;
	for (speed = 14; speed >= 1; speed--) {

		Printf("\rGoing at speed %d .. ", speed);

		TrainCommandSpeed(TRAIN, speed, train_tid);

		int module, sensor, time;
		struct track_node *sensor_node;

		int sample_time = 0;
		do {
			SensorGetNextTriggered(&module, &sensor, &time, sensor_tid);
			sensor_node = &track[SENSOR_TO_NODE(module,sensor)];

			if (!strcmp(sensor_node->name,"D7")) sample_time = time;
//			Printf("\rTriggered %s", sensor_node->name);
		} while (strcmp(sensor_node->name,"E8"));

		TrainCommandSpeed(TRAIN, 0, train_tid);
		Printf("Stopping train..\r");
		Printf("Press enter to reverse\r");
		Getc(COM2, uart_tid);

		TrainCommandReverse(TRAIN, train_tid);
		TrainCommandSpeed(TRAIN, 14, train_tid);
		do {
			SensorGetNextTriggered(&module, &sensor, &time, sensor_tid);
			sensor_node = &track[SENSOR_TO_NODE(module,sensor)];
//			Printf("\rTriggered %s", sensor_node->name);
		} while (strcmp(sensor_node->name,"D7"));
		TrainCommandSpeed(TRAIN, 0, train_tid);
		Delay(250, clock_tid);
		TrainCommandReverse(TRAIN, train_tid);

		Printf("Stopping train..");
	}

}
