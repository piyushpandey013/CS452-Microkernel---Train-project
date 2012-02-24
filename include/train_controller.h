#ifndef __TRAIN_CONTROLLER_H__
#define __TRAIN_CONTROLLER_H__

#include <track_node.h>
#include <track_data.h> // for TRACK_MAX
#include <location.h>

#include <train_profile.h>

#define TRAIN_CONTROLLER "train_controller"

struct train_controller_message {
	int command;
	char data[4];
	int offset;
};

struct train_position {
	int action;
	int sensor;
	int offset;
	int velocity;
	int estimated_time;
	int actual_time;
	int direction;
	int destination;
};

int TrainControllerSpeed(int train, int speed, int tid);
struct train_position TrainControllerPosition(int train, int tid);
int TrainControllerReverse(int train, int tid);
int TrainControllerReverseReset(int train, int tid);
int TrainControllerRoute(int train, int sensor_module, int blocking, int tid);
int TrainControllerRouteStop(int train, int tid);

#endif
