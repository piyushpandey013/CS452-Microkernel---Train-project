#include <syscall.h>
#include <io.h>

#include <name_server.h>
#include <sensor.h>
#include <train_command.h>
#include <uart_server.h>
#include <switch.h>
#include <clock.h>
#include <track_server.h>

//--------------------------Change these--------------------
#define TRAIN	36
//----------------------------------------------------------

void acceleration_calib_entry() {
	int sensor_tid = WhoIs(SENSOR_SERVER);
	int train_tid = WhoIs(TRAIN_COMMAND);
	int uart_tid = WhoIs(UART_SERVER);
	int clock_tid = WhoIs(CLOCK_SERVER);

	Printf("ACCELERATION CALIBRATION\r");
	Printf("------------------------\r\r");

	int i = 1;
	int speed = 14;

	TrainCommandGo(train_tid);

	while (1) {
		int distance = i * 50;

		Printf("Press any key to start measuring from %d...", distance);
		Getc(COM2, uart_tid);

		TrainCommandSpeed(TRAIN, speed, train_tid);
		int now = Time(clock_tid);

		int sensor_node, time;
		SensorGetNextTriggered(&sensor_node, &time, sensor_tid);

		int delta_t = (time - now);
		Printf("%d : %d\r", distance, delta_t);

		TrainCommandSpeed(TRAIN, 0, train_tid);

		i++;
	}

	Exit();
}
