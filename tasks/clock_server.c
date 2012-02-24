#include <io.h>
#include <syscall.h>

#include <ordered_list.h>
#include <list.h>
#include <event.h>
#include <timer.h>
#include <ts7200.h>

#include <name_server.h>
#include <clock.h>

#define MAX_TIMERS 12

#define TIMER3_VALUE (unsigned int*)(TIMER3_BASE + VAL_OFFSET)

extern void event_notifier_entry();

struct timer_node {
	struct ordered_list_node node;
	int tid;
};

// Message format for sending messages to the clock server
struct clock_event {
	int type;
	int data;
};

static void timers_init(struct ordered_list* timer_active_list, struct list *timer_free_list, struct timer_node* nodes) {

	AssertF(((int)&(timer_free_list->front)) % 4 == 0, "Not aligned4", 0);
	AssertF(((int)(nodes)) % 4 == 0, "Not aligned5; nodes = %x", nodes);
	timer_free_list->front = (struct list_node*)nodes;

	/* Point each block to the next except the last one */
	int i;
	for (i = 0; i < (MAX_TIMERS - 1); i++) {
		nodes[i].node.next = (struct ordered_list_node*)&nodes[i + 1];
	}
	nodes[i].node.next = 0;

	timer_free_list->back = (struct list_node*)&nodes[i];
	
	timer_active_list->front = 0;
	timer_active_list->back = 0;
}

void clock_server_entry() {
	//	bwprintf(COM2, "Clock server starting...\r\n");
	// Allocate space for the timers we need to use
	struct timer_node timers_free_nodes[MAX_TIMERS];

	// active and free timer lists
	struct ordered_list timers_list;
	struct list timers_free_list;
	
	// initialize the timer lists
	timers_init(&timers_list, &timers_free_list, timers_free_nodes);

	// Initialize the timer
	timer3_init();

	/* create our notifiers */
	// timer notifier:
	int notifier_tid = Create(PRIORITY_HIGHEST, &event_notifier_entry);
	int msg = EVENT_TIMER;
	Send(notifier_tid, (char*)&msg, sizeof(msg), (char*)&msg, sizeof(msg));

	// The sending task tid
	int requester;

	// Wait for our notifier to tell us he is ready`
	while (Receive(&requester, (char*)&msg, sizeof(msg)) && requester != notifier_tid) {
		Reply(requester, (char*)&msg, sizeof(msg));
	}
	// Tell the notifier to start AwaitEvent-ing
	Reply(notifier_tid, (char*)&msg, sizeof(msg));

	// Now tasks can find us and ask for the time :)
	RegisterAs("clock_server");

	// our running ticks, each tick is 10ms
	unsigned int ticks = 0;
	int previous_ticks = *TIMER3_VALUE;

	int notifier_blocked = 0;

	struct clock_event request;
	while (1) {
		Receive(&requester, (char*)&request, sizeof(request));

		/* Update the ticks value */
		int current_ticks = *TIMER3_VALUE;
		if (current_ticks > previous_ticks && current_ticks - previous_ticks >= 20) {
			ticks += (0xffffffff - (current_ticks - previous_ticks)) / 20;
			previous_ticks = current_ticks;
		} else if (previous_ticks - current_ticks >= 20) {
			ticks += (previous_ticks - current_ticks) / 20;
			previous_ticks = current_ticks;
		}

		if (requester == notifier_tid) {
			if (timers_list.front == 0) {
				// Nothing to process, so stop listening to events
				notifier_blocked = 1;
			} else {
				// Reply straight away to the notifier so it can continue
				// waiting for events
				int reply_value = 0;
				Reply(requester, (char*)&reply_value, sizeof(reply_value));

				struct timer_node * timer;
				while ((timer = (struct timer_node*)timers_list.front) != 0 && (timer->node.data <= ticks)) {
					// the time has passed, so we should wake up this task by replying
					// to it
					Reply(timer->tid, (char*)&reply_value, sizeof(reply_value));

					// remove the task's timer from the timer list
					ordered_list_pop_front(&timers_list);

					// and put it back on the free list to be reused
					list_push_back(&timers_free_list, (struct list_node *)timer);
				}
			}
		} else {
			// the received msg is not from the notifier!
			// which means the msg has a format. check what it is!
			switch ( request.type ) {
				struct timer_node *t;
				case CLOCK_TIME:
					Reply(requester, (char*)&ticks, sizeof(ticks));
					break;

				case CLOCK_DELAY:
					t = (struct timer_node*)list_pop_front(&timers_free_list);
					t->tid = requester;
					t->node.data = ticks + request.data;
					ordered_list_add(&timers_list, (struct ordered_list_node*)t);
					if (notifier_blocked) {
						int reply = 0;
						Reply(notifier_tid, (char*)&reply, sizeof(reply));
						notifier_blocked = 0;
					}
					break;

				case CLOCK_DELAY_UNTIL:
					t = (struct timer_node*)list_pop_front(&timers_free_list);
					t->tid = requester;
					t->node.data = request.data;
					ordered_list_add(&timers_list, (struct ordered_list_node*)t);
					if (notifier_blocked) {
						int reply = 0;
						Reply(notifier_tid, (char*)&reply, sizeof(reply));
						notifier_blocked = 0;
					}	
					break;

				default:
				//	bwprintf(COM2, "Unknown clock server request type %d\r\n", request.type);
				//	bwgetc(COM2);
					Exit();
					break;
			}
		}
	}
	Exit();
}

int Delay(int ticks, int tid) {
	struct clock_event msg;
	msg.type = CLOCK_DELAY;
	msg.data = ticks;
	int reply;
	int ret = Send(tid, (char*)&msg, sizeof(msg), (char*)&reply, sizeof(reply));
	if (ret == -1 || ret == -2) {
		return -1;
	}
	return reply;
}

int DelayUntil(int ticks, int tid) {
	struct clock_event msg;
	msg.type = CLOCK_DELAY_UNTIL;
	msg.data = ticks;
	int reply;
	int ret = Send(tid, (char*)&msg, sizeof(msg), (char*)&reply, sizeof(reply));
	if (ret == -1 || ret == -2) {
		return -1;
	}
	return reply;
}

int Time(int tid) {
	struct clock_event msg;
	msg.type = CLOCK_TIME;
	int reply;
	int ret = Send(tid, (char*)&msg, sizeof(msg), (char*)&reply, sizeof(reply));
	if (ret == -1 || ret == -2) {
		return -1;
	}
	return reply;
}

