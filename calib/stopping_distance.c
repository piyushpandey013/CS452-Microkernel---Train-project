#include <sensor.h>
#include <switch.h>
#include <term.h>
#include <train.h>
#include <uart_server.h>
#include <clock.h>
#include <train_command.h>
#include <io.h>
#include <name_server.h>

#include <track_data.h>
#include <track_node.h>
#include <string.h>

static int TRAIN;

void stopping_distance_task() {

	TRAIN = 36;

//	int term_tid = WhoIs("term_tid");
	int train_tid = WhoIs(TRAIN_COMMAND);
	int uart_tid = WhoIs(UART_SERVER);
	int sensor_tid = WhoIs(SENSOR_SERVER);
	int clock_tid = WhoIs(CLOCK_SERVER);

	Delay(100, clock_tid);
	TrainCommandSwitchStraight(16, train_tid);
	TrainCommandSwitchCurved(15, train_tid);
	TrainCommandSwitchStraight(9, train_tid);

	TrainCommandSpeed(TRAIN, 0, train_tid);

	int speed;
	for (speed = 14; speed >= 10; speed--) {
		Printf("\rPress enter to start at speed %d .. ", speed);
		Getc(COM2, uart_tid);

		TrainCommandSpeed(TRAIN, speed, train_tid);

		int time;
		int sensor_node;

		do {
			SensorGetNextTriggered(&sensor_node, &time, sensor_tid);
		} while (sensor_node != E8);

		TrainCommandSpeed(TRAIN, 0, train_tid);
	}
}
