#ifndef __SENSOR_H__
#define __SENSOR_H__

#include <clock.h>

// IPC message structure
struct sensor_report {
	char sensor_nodes[5];
	int num_sensors;
	int timestamp;
};

#define SENSOR_TO_NODE(module, sensor) ((16*(module)) + (sensor))
#define SENSOR_DELAY_AVG (90 MSEC)

// Associated tasks
int CreateSensorCourier(int priority);

// API methods
int SensorGetNextTriggered(int *sensor_node, int* time, int tid);

#define SENSOR_SERVER "sensor_server"

#endif
