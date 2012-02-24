#include <sensor.h>
#include <switch.h>
#include <term.h>
#include <train.h>
#include <uart_server.h>
#include <clock.h>
#include <train_command.h>
#include <train_controller.h>
#include <track_server.h>

#include <io.h>
#include <name_server.h>

#include <track_node.h>
#include <string.h>
#include <syscall.h>

static int TRAIN;

void velocity_task_entry() {
	TRAIN = 36;

//	int train_tid = WhoIs(TRAIN_COMMAND);

	int train_controller_tid = WhoIs(TRAIN_CONTROLLER);
//	int uart_tid = WhoIs(UART_SERVER);
	int sensor_tid = WhoIs(SENSOR_SERVER);
	int clock_tid = WhoIs(CLOCK_SERVER);
	int switch_tid = WhoIs(SWITCH_SERVER);

	int curspeed = 14;
	int time;

	Delay(10 SEC, clock_tid);
	TrainControllerSpeed(TRAIN, 5, train_controller_tid);
	Delay(10 SEC, clock_tid);

	for ( ; curspeed >= 10 ; --curspeed) {
		Printf("Running train %d at speed %d\r", TRAIN, curspeed);

		TrainControllerSpeed(TRAIN, curspeed, train_controller_tid);

		Delay(80 SEC, clock_tid);
    	SwitchSetDirection(16, SWITCH_STRAIGHT, switch_tid);
    	Printf("Switched sw16\r", TRAIN, curspeed);

    	Delay(60 SEC, clock_tid);
    	SwitchSetDirection(15, SWITCH_STRAIGHT, switch_tid);
    	Printf("Switched sw15\r", TRAIN, curspeed);

    	Delay(80 SEC, clock_tid);
    	SwitchSetDirection(6, SWITCH_STRAIGHT, switch_tid);
    	SwitchSetDirection(9, SWITCH_STRAIGHT, switch_tid);
    	Printf("Switched sw6 and sw9\r", TRAIN, curspeed);

    	Delay(60 SEC, clock_tid);
    	TrainControllerSpeed(TRAIN, curspeed, train_controller_tid);

    	int sensor_node;
    	do {
    		SensorGetNextTriggered(&sensor_node, &time, sensor_tid);
    	} while (sensor_node != A4);

    	TrainControllerSpeed(TRAIN, 0, train_controller_tid);
    	SwitchSetDirection(16, SWITCH_CURVED, switch_tid);
    	SwitchSetDirection(15, SWITCH_CURVED, switch_tid);
    	SwitchSetDirection(6, SWITCH_CURVED, switch_tid);
    	SwitchSetDirection(9, SWITCH_CURVED, switch_tid);

    	Delay(5 SEC, clock_tid);
	}

//	TrainControllerSpeed(TRAIN, 14, train_controller_tid);
//	Delay(5 SEC, clock_tid);
//
//	curspeed = 13;
//	for ( ; curspeed >= 8 ; --curspeed) {
//		Printf("Running train %d at speed %d (decelerating)\r", TRAIN, curspeed);
//
//		TrainControllerSpeed(TRAIN, curspeed, train_controller_tid);
//
//		Delay(60 SEC, clock_tid);
//    	SwitchSetDirection(16, SWITCH_STRAIGHT, switch_tid);
//    	Printf("Switched sw16\r", TRAIN, curspeed);
//
//    	Delay(45 SEC, clock_tid);
//    	SwitchSetDirection(15, SWITCH_STRAIGHT, switch_tid);
//    	Printf("Switched sw15\r", TRAIN, curspeed);
//
//    	Delay(60 SEC, clock_tid);
//    	SwitchSetDirection(6, SWITCH_STRAIGHT, switch_tid);
//    	SwitchSetDirection(9, SWITCH_STRAIGHT, switch_tid);
//    	Printf("Switched sw6 and sw9\r", TRAIN, curspeed);
//
//    	Delay(45 SEC, clock_tid);
//    	TrainControllerSpeed(TRAIN, curspeed, train_controller_tid);
//
//    	int sensor_node;
//    	do {
//    		SensorGetNextTriggered(&module, &sensor, &time, sensor_tid);
//    		sensor_node = SENSOR_TO_NODE(module,sensor);
//    	} while (sensor_node != A4);
//
//    	SwitchSetDirection(16, SWITCH_CURVED, switch_tid);
//    	SwitchSetDirection(15, SWITCH_CURVED, switch_tid);
//    	SwitchSetDirection(6, SWITCH_CURVED, switch_tid);
//    	SwitchSetDirection(9, SWITCH_CURVED, switch_tid);
//
//	}
//
//	TrainControllerSpeed(TRAIN, 0, train_controller_tid);

	Printf("DONE!\r");
	Exit();
}
