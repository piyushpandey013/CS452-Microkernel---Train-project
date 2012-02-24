#include <syscall.h>
#include <train_command.h>
#include <string.h>
#include <clist.h>
#include <switch.h>

#include <name_server.h>
#include <clock.h>

#define SOLENOID_DELAY 20

#define MAX_EVENT_QUEUE_SIZE 5

#define SWITCH_OP_CURVED 0
#define SWITCH_OP_STRAIGHT 1
#define SWITCH_OP_LISTEN 2
#define SWITCH_OP_GET 3

struct switch_command {
	char operation;
	char sw;
};

static char switch_table[256];

static void switch_server_init(int train_tid, int clock_tid) {
	// Initialize the sensor table
	memset(switch_table, SWITCH_CURVED, sizeof(switch_table));

	// Start the train controller in case it isn't started
	TrainCommandGo(train_tid);
	
	int i;
    for (i = 1; i <= 18; i++) {
    	if (i == 17) {
    		TrainCommandSwitchStraight(17, train_tid);
    		switch_table[17] = SWITCH_STRAIGHT;
    	} else if (i == 10) {
    		TrainCommandSwitchStraight(10, train_tid);
    		switch_table[10] = SWITCH_STRAIGHT;
    	} else {
    		TrainCommandSwitchCurved(i, train_tid);
    	}
    }

    TrainCommandSwitchCurved(0x9A, train_tid);
    TrainCommandSwitchCurved(0x9B, train_tid);
    TrainCommandSwitchStraight(0x9C, train_tid); // 156
    TrainCommandSwitchStraight(0x99, train_tid); // 153

    switch_table[0x99] = SWITCH_STRAIGHT;
    switch_table[0x9C] = SWITCH_STRAIGHT;

    Delay(SOLENOID_DELAY, clock_tid);
    TrainCommandSolenoidOff(train_tid);
}

void switch_server_entry() {
	RegisterAs(SWITCH_SERVER);
	
	int train_tid = WhoIs(TRAIN_COMMAND);
	int clock_tid = WhoIs(CLOCK_SERVER);
	
	// Set all switches to CURVED, except two in the middle of the track
	switch_server_init(train_tid, clock_tid);
	
	// Initialize the list of tid's to notify of switch events
	struct clist event_list;
	int event_list_buffer[MAX_EVENT_QUEUE_SIZE];
	clist_init(&event_list, event_list_buffer, MAX_EVENT_QUEUE_SIZE);
		
	int sender_tid;
	struct switch_command message;
	while (1) {
		Receive(&sender_tid, (char*)&message, sizeof(message));
		
		switch (message.operation) {
			int reply = 0;
			
			case SWITCH_OP_CURVED:
			case SWITCH_OP_STRAIGHT:
				// Reply straight away so we don't block
				Reply(sender_tid, (char*)&reply, sizeof(reply));

				if (message.operation == SWITCH_OP_CURVED && switch_table[(int)message.sw] != SWITCH_CURVED) {
					TrainCommandSwitchCurved(message.sw, train_tid);
					switch_table[(int)message.sw] = SWITCH_CURVED;
				} else if (message.operation == SWITCH_OP_STRAIGHT && switch_table[(int)message.sw] != SWITCH_STRAIGHT) {
					TrainCommandSwitchStraight(message.sw, train_tid);
					switch_table[(int)message.sw] = SWITCH_STRAIGHT;
				} else {
					// No change, so just exit
					break;
				}
				
				// A switch was changed, so make sure we notify people
				struct switch_report report;
				report.sw = message.sw;
				report.direction = switch_table[(int)message.sw];
				report.timestamp = Time(clock_tid);
				int tid;
				while (clist_pop(&event_list, &tid) == 1) {
					Reply(tid, (char*)&report, sizeof(report));
				}
				
				// Turn off the solenoid
				Delay(SOLENOID_DELAY, clock_tid);
				TrainCommandSolenoidOff(train_tid);
				
				break;
			
			case SWITCH_OP_LISTEN:
				AssertF(clist_push(&event_list, sender_tid) == 1, "Failed to register for switch events");
				break;
				
			case SWITCH_OP_GET: {
				int dir = (int)switch_table[(int)message.sw];
				Reply(sender_tid, (char*)&dir, sizeof(dir));
				} break;
			
			default:
				AssertF(0, "Invalid operation on switch server. sender_tid=%d,operation=%d", sender_tid, message.operation);
				reply = -1;
				Reply(sender_tid, (char*)&reply, sizeof(reply));
				break;
		}
	}
}

// The courier task associated with this server
static void switch_server_courier_entry() {
	int parent_tid = MyParentTid();
	int switch_tid = WhoIs("switch_server");

	int reply;
	struct switch_report report;
	struct switch_command msg;
	msg.operation = SWITCH_OP_LISTEN;
	while (1) {
		AssertF(Send(switch_tid, (char*)&msg, sizeof(msg), (char*)&report, sizeof(report)) > 0, "Could not listen to switch events");
		Send(parent_tid, (char*)&report, sizeof(report), (char*)&reply, sizeof(reply));
	}
}

int SwitchSetDirection(int sw, int direction, int tid) {
	int reply;
	struct switch_command msg;
	if (direction == SWITCH_CURVED) {
		msg.operation = SWITCH_OP_CURVED;
	} else if (direction == SWITCH_STRAIGHT) {
		msg.operation = SWITCH_OP_STRAIGHT;
	} else {
		return -1;
	}
	
	msg.sw = (char)sw;
	return Send(tid, (char*)&msg, sizeof(msg), (char*)&reply, sizeof(reply));
}

int SwitchGetDirection(int sw, int tid) {
	int reply;
	struct switch_command msg;
	msg.operation = SWITCH_OP_GET;
	msg.sw = (char)sw;
	
	if (Send(tid, (char*)&msg, sizeof(msg), (char*)&reply, sizeof(reply)) > 0) {
		return reply;
	}
	return -1;
}

int SwitchGetNextTriggered(int* sw, int* status, int* time, int tid) {
	struct switch_report report;
	struct switch_command msg;
	msg.operation = SWITCH_OP_LISTEN;
	if (Send(tid, (char*)&msg, sizeof(msg), (char*)&report, sizeof(report)) > 0) {
		*sw = (int)report.sw;
		*status = (int)report.direction;
		*time = report.timestamp;
		return 0;
	}
	return -1;
}

int CreateSwitchCourier(int priority) {
	return Create(priority, &switch_server_courier_entry);
}
