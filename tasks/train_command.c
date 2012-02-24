#include <syscall.h>
#include <train_command.h>
#include <bwio.h>

#include <name_server.h>
#include <uart_server.h>
#include <clock.h>

#define TRAIN_GO				0
#define TRAIN_STOP				1
#define TRAIN_SPEED				2
#define TRAIN_REVERSE			3
#define TRAIN_SWITCH_STRAIGHT 	4
#define TRAIN_SWITCH_CURVED 	5
#define TRAIN_SOLENOID_OFF		6
#define TRAIN_READ_SENSOR		7
#define TRAIN_READ_SENSORS		8
#define TRAIN_RESET_SENSORS 	9

struct train_command_message {
	char command;
	char data1;
	char data2;
};

void train_command_entry() {
	RegisterAs(TRAIN_COMMAND);
	
	int clock_tid = WhoIs(CLOCK_SERVER);
	int uart_tid = WhoIs(UART_SERVER);

	int sender_tid;
	struct train_command_message msg;
	while (1) {
		int reply = 0;

		Receive(&sender_tid, (char*)&msg, sizeof(msg));

		switch (msg.command) {
			case TRAIN_GO:
				Putc(COM1, 0x60, uart_tid);
				break;

			case TRAIN_STOP:
				Putc(COM1, 0x61, uart_tid);
				break;

			case TRAIN_SPEED:
				if (msg.data1 <= 14) {
					if (msg.data2 > 0 && msg.data2 <= 80) {
						Putc(COM1, msg.data1 | 16, uart_tid);
						Putc(COM1, msg.data2, uart_tid);
						break;
					}
				}
				// Error case
				reply = -1;
				break;

			case TRAIN_REVERSE:
				if (msg.data1 > 0 && msg.data1 <= 80) {
					Putc(COM1, 0x0F, uart_tid);
					Putc(COM1, msg.data1, uart_tid);
					break;
				}
				// Error case
				reply = -1;
				break;

			case TRAIN_READ_SENSORS:
				if (msg.data1 > 0 && msg.data1 <= 31) {
					Putc(COM1, 0x80 + msg.data1, uart_tid);
					break;
				}
				// Error case
				reply = -1;
				break;

			case TRAIN_RESET_SENSORS:
				if (msg.data1 > 0 && msg.data1 <= 31) {
					Putc(COM1, 192, uart_tid);
					break;
				}
				// Error case
				reply = -1;
				break;

			case TRAIN_SWITCH_CURVED:
				Putc(COM1, 0x22, uart_tid);
				Putc(COM1, msg.data1, uart_tid);
				break;

			case TRAIN_SWITCH_STRAIGHT:
				Putc(COM1, 0x21, uart_tid);
				Putc(COM1, msg.data1, uart_tid);
				break;

			case TRAIN_SOLENOID_OFF:
				Putc(COM1, 0x20, uart_tid);
				break;

			default:
				reply = -1;
				break;
		}

		// delay between train commands
		Delay(1, clock_tid); // 1 tick = 10ms

		Reply(sender_tid, (char*)&reply, sizeof(reply));
	}

	Exit();
}

/* ALL THE TIDS ARE TRAIN TIDS */
int TrainCommandGo(int tid) {
	int reply;
	struct train_command_message msg;
	msg.command = TRAIN_GO;
	Send(tid, (char*)&msg, sizeof(msg), (char*)&reply, sizeof(reply));
	return reply;
}

int TrainCommandStop(int tid) {
	int reply;
	struct train_command_message msg;
	msg.command = TRAIN_STOP;
	Send(tid, (char*)&msg, sizeof(msg), (char*)&reply, sizeof(reply));
	return reply;
}

int TrainCommandSpeed(char train, char speed, int tid) {
	int reply;
	struct train_command_message msg;
	msg.command = TRAIN_SPEED;
	msg.data1 = speed;
	msg.data2 = train;
	Send(tid, (char*)&msg, sizeof(msg), (char*)&reply, sizeof(reply));
	return reply;
}

int TrainCommandReverse(char train, int tid) {
	int reply;
	struct train_command_message msg;
	msg.command = TRAIN_REVERSE;
	msg.data1 = train;
	Send(tid, (char*)&msg, sizeof(msg), (char*)&reply, sizeof(reply));
	return reply;
}

int TrainCommandResetSensors(int tid) {
	int reply;
	struct train_command_message msg;
	msg.command = TRAIN_RESET_SENSORS;
	Send(tid, (char*)&msg, sizeof(msg), (char*)&reply, sizeof(reply));
	return reply;
}

int TrainCommandReadSensors(int modules, int tid) {
	int reply;
	struct train_command_message msg;
	msg.command = TRAIN_READ_SENSORS;
	msg.data1 = modules;
	Send(tid, (char*)&msg, sizeof(msg), (char*)&reply, sizeof(reply));
	return reply;
}

int TrainCommandSwitchCurved(int sw, int tid) {
	int reply;
	struct train_command_message msg;
	msg.command = TRAIN_SWITCH_CURVED;
	msg.data1 = sw;
	Send(tid, (char*)&msg, sizeof(msg), (char*)&reply, sizeof(reply));
	return reply;
}

int TrainCommandSwitchStraight(int sw, int tid) {
	int reply;
	struct train_command_message msg;
	msg.command = TRAIN_SWITCH_STRAIGHT;
	msg.data1 = sw;
	Send(tid, (char*)&msg, sizeof(msg), (char*)&reply, sizeof(reply));
	return reply;
}

int TrainCommandSolenoidOff(int tid) {
	int reply;
	struct train_command_message msg;
	msg.command = TRAIN_SOLENOID_OFF;
	Send(tid, (char*)&msg, sizeof(msg), (char*)&reply, sizeof(reply));
	return reply;
}
