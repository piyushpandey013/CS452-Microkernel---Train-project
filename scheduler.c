#include <task.h>
#include <task_queue.h>
#include <scheduler.h>
#include <event.h>
#include <string.h>
#include <syscall_kern.h>

static struct task_queue ready_queue[PRIORITY_HIGHEST + 1];
static struct task_queue event_queue[STATE_EVENT_END - STATE_EVENT_START + 1];
static struct task_queue defunct_queue;
static int events_waiting;
/*
 * priorities:
 *  1 - 9
 * states:
 * 	refer to http://www.cgl.uwaterloo.ca/~wmcowan/teaching/cs452/f11/notes/l08.html:
 *    STATE_DEFUNCT, STATE_READY, STATE_ACTIVE, STATE_SEND_BLOCKED, STATE_RECEIVE_BLOCKED, STATE_REPLY_BLOCKED
 *    defined in scheduler.h
 */

#include <bwio.h>

void scheduler_init() {
	events_waiting = 0;
	memset((void*)ready_queue, 0, sizeof(ready_queue));
	memset((void*)event_queue, 0, sizeof(event_queue));
	memset((void*)&defunct_queue, 0, sizeof(defunct_queue));
}

int scheduler_add(struct task *task) {
	if (task->priority < PRIORITY_IDLE || task->priority > PRIORITY_HIGHEST) {
		return -1;
	}

	if (task->state >= STATE_EVENT_START && task->state <= STATE_EVENT_END) {
		int eventIndex = task->state - STATE_EVENT_START;
		task_queue_push_back(&event_queue[eventIndex], task);
		events_waiting++;
	} else {
		switch (task->state) {
			case STATE_READY:
				/* Add this task to the ready priority queues, as only processes which are ready can be activated */
				task_queue_push_back(&ready_queue[task->priority], task);
				break;
			
			case STATE_DEFUNCT:
				task_queue_push_back(&defunct_queue, task);
				break;

			case STATE_SEND_BLOCKED:
			case STATE_RECEIVE_BLOCKED:
			case STATE_REPLY_BLOCKED:
				break;		
	
			default:	
				sys_assert(task, "Adding task that doesn't have recognized state");		
				return -1;
				break;
		}
	}
	return 0;
}

/* Removes and returns the highest priority task on the ready queues */
struct task * scheduler_get_next() {
	int priority = PRIORITY_HIGHEST;
	for (; priority >= PRIORITY_LOWEST; priority--) {
		if (ready_queue[priority].front != 0) {
			return task_queue_pop_front(&ready_queue[priority]);
		}
	}
	
	if (events_waiting == 0) {
		// No events are waiting in the queues, so there's no need
		// to keep the kernel going
		return 0;
	}

	return task_queue_pop_front(&ready_queue[PRIORITY_IDLE]);
}

/* Removes and returns the first task on the queue that is blocked on the
 * specified event
 */
struct task * scheduler_get_task_blocked_on_event(int eventId) {
	struct task* t = task_queue_pop_front(&event_queue[eventId]);
	if (t != 0) {
		events_waiting--;
	}
	return t;
}

void scheduler_print_queues(int port) {
	int priority = PRIORITY_HIGHEST;
	for (; priority >= PRIORITY_LOWEST; priority--) {
		bwprintf(port, "%d: ", priority);
		struct task* t = ready_queue[priority].front;
		while (t) {
			bwprintf(port, "%d, ", t->tid);
			t = t->next;
		}
		bwputc(port, '\r');
	}
}

void scheduler_print_event_queue(int port, int eventId) {
	struct task* t = event_queue[eventId].front;
	while (t) {
		bwprintf(port, "%d, ", t->tid);
		t = t->next;
	}
	bwputc(port, '\r');
}	

