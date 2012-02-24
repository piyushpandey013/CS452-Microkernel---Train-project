#ifndef __SWITCH_H__
#define __SWITCH_H__

#include <track_node.h>
#define SWITCH_SERVER "switch_server"

#define SWITCH_CURVED DIR_CURVED
#define SWITCH_STRAIGHT DIR_AHEAD

// IPC message structure
struct switch_report {
	char sw;
	char direction;
	int timestamp;
};

// API method
int SwitchSetDirection(int sw, int direction, int tid);
int SwitchGetDirection(int sw, int tid);
int SwitchGetNextTriggered(int* sw, int* status, int* timestamp, int tid);

// Associated tasks
int CreateSwitchCourier(int priority);

#endif
