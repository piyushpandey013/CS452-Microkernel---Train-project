#include <syscall.h>
#include <train_command.h>
#include <train_controller.h>
#include <clock.h>
#include <name_server.h>

#define TRAIN_SPEED 0
#define TRAIN_STOP 1
#define TRAIN_REVERSE 2

struct train_message {
	int command;
	int data;
};

void train_task_entry() {

	int train_tid = WhoIs(TRAIN_COMMAND);
	int clock_tid = WhoIs(CLOCK_SERVER);
	int train_controller_tid = WhoIs(TRAIN_CONTROLLER);
	int train = -1;

	int sending_tid;
	struct train_message msg;
	int reply_ok = 1;

	Receive(&sending_tid, (char*)&train, sizeof(train));
	AssertF(sending_tid == MyParentTid(), "Message received from source other than parent");
	AssertF(train > 0, "Train is invalid: %d", train);
	Reply(sending_tid, (char*)&reply_ok, sizeof(reply_ok));

	while (1) {
		Receive(&sending_tid, (char*)&msg, sizeof(msg));
		Reply(sending_tid, (char*)&reply_ok, sizeof(reply_ok));

		switch (msg.command) {
			case TRAIN_SPEED:
				AssertF(msg.data >= 0 && msg.data <= 14, "Invalid speed %d", msg.data);
				TrainCommandSpeed(train, msg.data, train_tid);
				break;

			case TRAIN_STOP:
				Delay(msg.data, clock_tid);

				// Tell the train controller to stop this train, so that the deceleration is
				// recorded
				TrainControllerSpeed(train, 0, train_controller_tid);
				break;

			case TRAIN_REVERSE: {
				int old_speed = msg.data;
				AssertF(old_speed >= 0 && old_speed <= 14, "Invalid Old speed %d", old_speed);

				Delay(4 SEC, clock_tid);
				TrainCommandReverse(train, train_tid);
				TrainControllerReverseReset(train, train_controller_tid);
				TrainControllerSpeed(train, old_speed, train_controller_tid);
			} break;

			default:
				AssertF(0, "invalid command to train task! sending_tid=%d, cmd=%d", sending_tid, msg.command);
		}
	}

	Exit();
}

int CreateTrainTask(int priority, int train) {
	int tid = Create(priority, &train_task_entry);
	int reply;
	Send(tid, (char*)&train, sizeof(train), (char*)&reply, sizeof(reply));
	return tid;
}

int TrainSpeed(int speed, int tid) {
	struct train_message msg = {TRAIN_SPEED, speed};
	int reply;
	return Send(tid, (char*)&msg, sizeof(msg), (char*)&reply, sizeof(reply));
}

int TrainStop(int delay, int tid) {
	struct train_message msg = {TRAIN_STOP, delay};
	int reply;
	return Send(tid, (char*)&msg, sizeof(msg), (char*)&reply, sizeof(reply));
}

int TrainReverse(int old_speed, int tid) {
	struct train_message msg = {TRAIN_REVERSE, old_speed};
	int reply;
	return Send(tid, (char*)&msg, sizeof(msg), (char*)&reply, sizeof(reply));
}

