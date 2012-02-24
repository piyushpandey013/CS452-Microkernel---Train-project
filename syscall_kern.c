#include <syscall_kern.h>
#include <syscall.h>
#include <task.h>
#include <scheduler.h>
#include <interrupts.h>
#include <uart.h>
#include <ts7200.h>
#include <string.h>

#include <bwio.h>
#include <debug.h>

#include <term.h> // for the cursor constants

int sys_create(struct task *t, int priority, void (*code)()) {
	if (priority == PRIORITY_IDLE) {
		return -1;
	}

	struct task *new_task = task_create(code, priority, t->tid);

	if ((int)new_task < 0) {
		KASSERT(0, "Could not create new task. Error code: %d", (int)new_task);
		return (int)new_task;
	}
	
	scheduler_add(new_task);
	
	return new_task->tid;
}

int sys_my_tid(struct task *t) {
	return t->tid;
}

int sys_my_parent_tid(struct task *t) {
	return t->parent_tid;
}

void sys_exit(struct task *t) {
	t->state = STATE_DEFUNCT;
}

/*
 *  message passing stuff
 */

int sys_send(struct task *t, int tid, char *msg, int msglen, char *reply, int replylen) {
	KASSERT(t->state == STATE_READY, "send int task %d: sending task is not in ready state", t->tid);
	
	// Check if the task we are sending to exists
	struct task *receiving_task = task_get_by_tid(tid);

	if (receiving_task == 0) {
		KASSERT(0, "send in task %d: sending to invalid task %d", t->tid, tid);
		return -1;
	} else if (receiving_task->state == STATE_UNUSED) {
		KASSERT(0, "send in task %d: sending to unused task %d", t->tid, tid);
		return -2;
	} else if (receiving_task->state == STATE_DEFUNCT) {
		KASSERT(0, "send in task %d: sending to defunct task %d", t->tid, tid);
		return -3;
	} else if (replylen == 0) {
		KASSERT(0, "send in task %d: replylen is 0!", t->tid);
		return -4;
	}

	// we can now store the message and the reply stuff!
	t->state = STATE_RECEIVE_BLOCKED;

	// add this queue to the receiver's send queue!
	task_queue_push_back(&(receiving_task->send_queue), t);

	// if receiver is send blocked, copy the message over and put him up!
	if (receiving_task->state == STATE_SEND_BLOCKED) {
		struct user_registers *receiving_reg = (struct user_registers*)(receiving_task->sp - sizeof(struct user_registers));
		receiving_reg->r0 = sys_receive(receiving_task, (int*)receiving_reg->r0, (char*)receiving_reg->r1, receiving_reg->r2);
		scheduler_add(receiving_task);
	}

	// preserve the r0 register! (it is both the return value and the first argument of Send())
	// it will be replaced by Reply()!
	return tid;
}

int sys_receive(struct task *t, int *tid, char *msg, int msglen) {
	if (t->send_queue.front == 0) {
		t->state = STATE_SEND_BLOCKED;
		// preserve the r0 register (it is both the return value and the first argument)!
		// it will be replaced later by sys_send().
		return (int)tid;
	} else {
		struct task *sending_task = task_queue_pop_front(&(t->send_queue));
		
		KASSERT(sending_task->state == STATE_RECEIVE_BLOCKED, 
				"receive in task %d: receiving message from non sending task %d", t->tid, sending_task->tid);
		
		struct user_registers *sending_reg = (struct user_registers*)(sending_task->sp - sizeof(struct user_registers));
		/*
		 * From Send(), we see that:
		 * 	sending_reg->r0 = int tid		(that's us!)
		 * 	sending_reg->r1 = char *msg 	(the message that was sent)
		 * 	sending_reg->r2 = int msglen 	(the byte count of the message that was sent)
		 * 	sending_reg->r3 = char *reply	(where to store the reply to this message)
		 * 	sending_reg->r4 = int replylen 	(byte count of the reply memory)
		 */

		// copy over the tid and the msg
		int len_copied = MIN(msglen, sending_reg->r2);
		*tid = sending_task->tid;
		memcpy(msg, (char*)sending_reg->r1, len_copied);

		sending_task->state = STATE_REPLY_BLOCKED;
		t->state = STATE_READY;

		return sending_reg->r2;
	}

}

int sys_reply(struct task *t, int tid, char *reply, int replylen) {
	/*
	 * From Send(), we see that:
	 * 	waiting_reg->r0 = int tid		(the task receiving the sent message)
	 * 	waiting_reg->r1 = char *msg 	(the message that was sent)
	 * 	waiting_reg->r2 = int msglen 	(the byte count of the message that was sent)
	 * 	waiting_reg->r3 = char *reply	(where to store the reply to this message)
	 * 	waiting_reg->r4 = int replylen 	(byte count of the reply memory)
	 */
	
	// Check if the task we are sending to exists
	struct task *waiting_task = task_get_by_tid(tid);

	if (waiting_task == 0) {
		KASSERT(0, "reply in task %d: replying to invalid tid %d", t->tid, tid);
		return -1;
	} else if (waiting_task->state == STATE_UNUSED || waiting_task->state == STATE_DEFUNCT) {
		KASSERT(0, "reply in task %d: State is unused or defunct", t->tid);
		return -2;
	} else if (waiting_task->state != STATE_REPLY_BLOCKED) {
		KASSERT(0, "reply in task %d: Reply blocked task's state is not reply blocked. Replying to %d\r", t->tid, tid);
		return -3;
	}
	struct user_registers *waiting_reg = (struct user_registers*)(waiting_task->sp - sizeof(struct user_registers));
	if (waiting_reg->r4 < replylen) {
		KASSERT(0, "reply in %d: Could not reply: reply size is %d and space is %d", t->tid, replylen, waiting_reg->r4);
		return -4; // insufficient space to store reply!
	}

	// copy over the reply
	memcpy((char*)waiting_reg->r3, reply, replylen);

	// the waiting task that originally did the Send() can now run!
	waiting_task->state = STATE_READY;
	waiting_reg->r0 = replylen; // the Sending task returns our replylen!
	scheduler_add(waiting_task);

	return 0; // success!
}

int sys_await_event(struct task *t, int eventId) {
	if (eventId < EVENT_START || eventId > EVENT_END) {
		return -1;
	}

	// Set the task to be blocked on the requested event
	t->state = eventId + STATE_EVENT_START;

	// Based on the event type, we must turn on the appropriate
	// interrupts. This is done so that we don't waste time dealing
	// with interrupts we don't care about and to allow user tasks
	// to write to the UART unburdened by constant interrupts
	interrupts_enable(eventId);

	// Returning zero here is not representitive of the actual result of this call.
	// When the user task is woken up due to the triggered event, it will have a
	// different return value, depending on what happens in the HWI
	return 0;
}

/*******************************/
struct name_record {
	char name[128];
	int tid;
	struct name_record* next;
};
void * name_server_get_active_list();
int getNumMissedBytes();

/********************************/

int sys_assert(struct task *t, char* message) {
	uart_disable_int(COM2);

	bwprintf(COM2, "\x1b[%d;%dr", 0, 999); // reset scrolling
	bwprintf(COM2, "\rASSERT FAILED: %s\r", message);

	/* name server stuff */
	bwprintf(COM2, "servers: ");
	struct name_record *name = name_server_get_active_list();
	while (name != 0) {
		bwprintf(COM2, " [%s=%d] ", name->name, name->tid);
		name = name->next;
	}
	bwprintf(COM2, "\r");

	if (t != 0) {
		struct user_registers *waiting_reg = (struct user_registers*)(t->sp - sizeof(struct user_registers));
		bwprintf(COM2, "\rCurrent running task: %d\r", t->tid);
		bwprintf(COM2, "[PC: %x] " "[LR: %x] " " [SP: %x]\r", t->pc, waiting_reg->lr, t->sp);
	}

	int i;
	for (i = 0; i < MAX_NUM_TASKS; i++) {
		task_print(COM2, i);
	}

	scheduler_print_queues(COM2);

	bwprintf(COM2, "EVENT BLOCKED\r");
	bwprintf(COM2, "TIMER: ");
	scheduler_print_event_queue(COM2, EVENT_TIMER);	
	bwprintf(COM2, "UART1_RX: ");
	scheduler_print_event_queue(COM2, EVENT_UART1_RX);	
	bwprintf(COM2, "UART1_TX: ");
	scheduler_print_event_queue(COM2, EVENT_UART1_TX);	
	bwprintf(COM2, "UART2_RX: ");
	scheduler_print_event_queue(COM2, EVENT_UART2_RX);	
	bwprintf(COM2, "UART2_TX: ");
	scheduler_print_event_queue(COM2, EVENT_UART2_TX);	

	bwprintf(COM2, "BYTES MISSED ON COM1: %d\r", getNumMissedBytes());

	bwprintf(COM2, "\rPress any key to continue...");

	int c = *UART2_DATA;
	c = bwgetc(COM2);

	bwprintf(COM2, "\r");

	uart_enable_int(COM2);
	return 0;
}

int sys_handle_request(struct task * t, int request) {
	struct user_registers *ureg = (struct user_registers *)(t->sp - sizeof(struct user_registers));
	
	switch (request) {
		case SYS_CREATE:
			ureg->r0 = sys_create(t, ureg->r0, (void (*)())ureg->r1);
			break;
			
		case SYS_MYTID:
			ureg->r0 = sys_my_tid(t);
			break;
			
		case SYS_MYPARENTTID:
			ureg->r0 = sys_my_parent_tid(t);
			break;
			
		case SYS_PASS:
			break;

		case SYS_EXIT:
			sys_exit(t);
			break;

		case SYS_SEND:
			ureg->r0 = sys_send(t, ureg->r0, (char*)ureg->r1, ureg->r2, (char*)ureg->r3, ureg->r4);
			break;

		case SYS_RECEIVE:
			ureg->r0 = sys_receive(t, (int*)ureg->r0, (char*)ureg->r1, ureg->r2);
			break;

		case SYS_REPLY:
			ureg->r0 = sys_reply(t, ureg->r0, (char*)ureg->r1, ureg->r2);
			break;

		case SYS_AWAIT_EVENT:
			ureg->r0 = sys_await_event(t, ureg->r0);
			break;

		case SYS_ASSERT:
			ureg->r0 = sys_assert(t, (char*)ureg->r0);
			break;

		case SYS_SHUTDOWN:
			return -1;
			break;

		default:
			KASSERT(0, "Illegal system call from tid %d: code %d", t->tid, request);
			break;
	}
	
	scheduler_add(t);
	return 0;
}
