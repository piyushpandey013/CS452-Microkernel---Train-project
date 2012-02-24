
#include <bwio.h>
#include <debug.h>

#include <task.h>
#include <scheduler.h>

extern void user_mode(volatile struct task *);

/* Allocates space for all the task descriptors and task stacks */
static struct task all_tasks[MAX_NUM_TASKS];
static char all_stacks[MAX_NUM_TASKS * STACK_SIZE];

/* Holds the next available tid */
static int current_free_tid;

/* Initializes the tasks in memory for kernel use */
void task_init() {
	current_free_tid = 0;
	int i;
	for (i = 0; i < MAX_NUM_TASKS; i++) {
		all_tasks[i].state = STATE_UNUSED;
	}
}

/*
 * sets up task `t`:
 *  - set the CPSR
 *  - allocate memory for user task
 *  - initialize registers into the stack to be restore.
 */
struct task* task_create(void (*fn)(), int priority, int parent_tid) {
	// At this point, CPSR = 0x600000d3 (observed by printing it out)

	if (priority < PRIORITY_IDLE || priority > PRIORITY_HIGHEST) {
		return (struct task*)-1;
	}
	
	if (current_free_tid >= MAX_NUM_TASKS) {
		kassert(0, "Ran out of free task tids!");
		return (struct task*)-2;
	}
	

	/* Find and init the next tid block */
	struct task* task = &(all_tasks[current_free_tid]);
	task->tid = current_free_tid++;

	task->parent_tid = parent_tid;
	task->priority = priority;
	task->state = STATE_READY;
	task->pc = (void*)fn;
	/* Make sure the Program status register is clean and set to user mode */
	//	task->cpsr_usr = 0x600000d3 & 0xE0;

	/*
	 * 0x60000050 -> 0b1100000000000000000000001010000
	 * this sets M[0:4] to User mode, and sets the I (disable-IRQ) bit to 0.
	 */
	task->cpsr_usr = 0x60000050;

	/* Find the task's stack's block of memory.
	   NOTE: The stack initially points just above its allotted space, since anything pushed
	   on the stack will go below this spot. Writing to this current stack pointer will write
	   out of bounds. 
	*/
	task->sp = all_stacks + ((task->tid + 1) * STACK_SIZE);

	/* Switching into a user task will unroll registers off its stack, including the stack pointer.
	   We need to set the same stack pointer we set above into the stack space at the correct location.
	*/

	struct user_registers *ureg = (struct user_registers *)(task->sp - sizeof(struct user_registers));
	ureg->sp = (int)task->sp;

	/* init message passing stuff */
	task->send_queue.front = 0;
	task->send_queue.back = 0;

	return task;
}


// returns the 'n' in swi <n>
int task_activate(volatile struct task *t) {
	user_mode(t);
	
	/*
	 * we have the masked 'n' code in (lr_svc - 4)!
	 * we have the other arguments of the swi interrupt in the user task. use the task descriptor for this!
	 */

	 // mask the last 24 bits of the swi instruction to get the code
	return *((int*)(t->pc) - 1) & (0xffffff);
}


// given a tid, returns the task structure
struct task * task_get_by_tid(int tid) {
	if (tid < 0 || tid >= MAX_NUM_TASKS) {
		return 0;
	}
	return &(all_tasks[tid]);
}

// get the first non-defunct task with parent tid `parent_tid`
struct task * task_get_first_by_parent_tid(int parent_tid) {
	if (parent_tid < 0  || parent_tid >= MAX_NUM_TASKS) {
		return 0;
	}

	int i;
	for (i = 0; i < MAX_NUM_TASKS; ++i) {
		if ((all_tasks[i].parent_tid == parent_tid)
				&& (all_tasks[i].state != STATE_DEFUNCT)
		) {
			return &all_tasks[i];
		}
	}

	return 0;
}

struct task * task_get_first_by_state(int state) {
	if (state > STATE_EVENT_END || state < STATE_DEFUNCT) {
		return 0;
	}

	int i;
	for (i = 0; i < MAX_NUM_TASKS; ++i) {
		if (all_tasks[i].state == state) {
			return &all_tasks[i];
		}
	}

	return 0;
}

void task_print(int port, int tid) {
	struct task* t = task_get_by_tid(tid);

	if (t == 0) {
		return;
	}

	struct user_registers *reg = (struct user_registers*)(t->sp - sizeof(struct user_registers));

	bwprintf(port, "Task %d -> ", t->tid);
	switch (t->state) {
	case STATE_READY:
		bwprintf(port, "READY");
		break;

	case STATE_UNUSED:
		bwprintf(port, "UNUSED");
		break;

	case STATE_DEFUNCT:
		bwprintf(port, "DEFUNCT");
		break;

	case STATE_ACTIVE:
		bwprintf(port, "ACTIVE");
		break;

	case STATE_SEND_BLOCKED:
		bwprintf(port, "SEND BLOCKED");
		break;

	case STATE_RECEIVE_BLOCKED:
		bwprintf(port, "RECEIVE BLOCKED ON %d", reg->r0);
		break;

	case STATE_REPLY_BLOCKED:
		bwprintf(port, "REPLY BLOCKED ON %d", reg->r0);
		break;

	case STATE_EVENT_BLOCKED_TIMER:
		bwprintf(port, "EVENT BLOCKED TIMER");
		break;

	case STATE_EVENT_BLOCKED_UART1_RX:
		bwprintf(port, "EVENT BLOCKED UART1 RX");
		break;

	case STATE_EVENT_BLOCKED_UART1_TX:
		bwprintf(port, "EVENT BLOCKED UART1 TX");
		break;

	case STATE_EVENT_BLOCKED_UART2_RX:
		bwprintf(port, "EVENT BLOCKED UART2 RX");
		break;

	case STATE_EVENT_BLOCKED_UART2_TX:
		bwprintf(port, "EVENT BLOCKED UART2 TX");
		break;
	
	default:
		bwprintf(port, "UNKNOWN %d", t->state);
		break;
	}

	int bottom = (int)(all_stacks + ((t->tid + 1) * STACK_SIZE));
	bwprintf(port, " STACK: %d/%d\r", bottom - (int)t->sp, STACK_SIZE);
}

