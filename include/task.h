#ifndef __TASK_H__
#define __TASK_H__

#include <task_queue.h>
#include <event.h>

#define MAX_NUM_TASKS 40

// stack size in bytes
#define STACK_SIZE (1024 * 4)

#define STATE_UNUSED -1
#define STATE_DEFUNCT 0 
#define STATE_READY 1
#define STATE_ACTIVE 2
#define STATE_SEND_BLOCKED 3
#define STATE_RECEIVE_BLOCKED 4
#define STATE_REPLY_BLOCKED 5

/* Event blocked states */
#define STATE_EVENT_START 6
#define STATE_EVENT_BLOCKED_TIMER STATE_EVENT_START + EVENT_TIMER
#define STATE_EVENT_BLOCKED_UART1_RX STATE_EVENT_START + EVENT_UART1_RX
#define STATE_EVENT_BLOCKED_UART1_TX STATE_EVENT_START + EVENT_UART1_TX
#define STATE_EVENT_BLOCKED_UART2_RX STATE_EVENT_START + EVENT_UART2_RX
#define STATE_EVENT_BLOCKED_UART2_TX STATE_EVENT_START + EVENT_UART2_TX
#define STATE_EVENT_END STATE_EVENT_BLOCKED_UART2_TX

struct task {
	
	// don't add anything inbetween this block of members, or you'll be screwing with assembly!
	// ------------START OF POSITION DEPENDANT MEMBERS-----------------
	int tid;
	unsigned int cpsr_usr;
	char* sp;
	void* pc;
	// -------------END OF POSITION DEPENDANT MEMBERS-------------------

	int state;
	int priority;	
	int parent_tid;
	struct task* next;

	// message passing related members
	struct task_queue send_queue; // contains queue of tasks that we have to receive
};

struct user_registers {
	int r0;
	int r1;
	int r2;
	int r3;
	int r4;
	int r5;
	int r6;
	int r7;
	int r8;
	int r9;
	int r10;
	int fp;
	int r12;
	int sp;
	int lr;
};

void task_init();
struct task* task_create(void (*fn)(), int priority, int parent_tid);
int task_activate(volatile struct task *t);
struct task * task_get_by_tid(int tid);
struct task * task_get_first_by_parent_tid(int parent_tid);
struct task * task_get_first_by_state(int state);

void task_print(int port, int tid);

#endif
