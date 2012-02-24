#include <sensor.h>
#include <switch.h>
#include <term.h>
#include <train.h>
#include <io.h>

#include <track_data.h>
#include <track_node.h>
#include <server_tasks.h>
#include <string.h>

static int TRAIN;

static struct track_node track[TRACK_MAX];

void stopping_distance_task() {

	TRAIN = 38;


//	int term_tid = WhoIs("term_tid");
	int train_tid = WhoIs("train_server");
	int uart_tid = WhoIs("uart_server");
	int sensor_tid = WhoIs("sensor_server");
	int clock_tid = WhoIs("clock_server");

	init_tracka(track);

	Delay(100, clock_tid);
	TrainCommandSwitchStraight(16, train_tid);
	TrainCommandSwitchCurved(15, train_tid);
	TrainCommandSwitchStraight(9, train_tid);

	Printf("press 'a' for train 37, 'b' for 38\r");
	if (Getc(COM2, uart_tid) == 'a') TRAIN = 37;
	else TRAIN = 38;

	TrainCommandSpeed(TRAIN, 0, train_tid);

	int speed;
	for (speed = 14; speed >= 1; speed--) {

		Printf("\rPress enter to start at speed %d .. ", speed);
		Getc(COM2, uart_tid);

		TrainCommandSpeed(TRAIN, 14, train_tid);

		int module, sensor, time;
		struct track_node *sensor_node;

		int sample_time = 0;
		do {
			SensorGetNextTriggered(&module, &sensor, &time, sensor_tid);
			sensor_node = &track[SENSOR_TO_NODE(module,sensor)];

//			Printf("\rTriggered %s", sensor_node->name);
		} while (strcmp(sensor_node->name,"D14"));

		TrainCommandSpeed(TRAIN, speed, train_tid);

		do {
			SensorGetNextTriggered(&module, &sensor, &time, sensor_tid);
			sensor_node = &track[SENSOR_TO_NODE(module,sensor)];
//			Printf("\rTriggered %s", sensor_node->name);
		} while (strcmp(sensor_node->name,"E8"));
		TrainCommandSpeed(TRAIN, 0, train_tid);

	}

}
