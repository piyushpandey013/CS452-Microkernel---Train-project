#include <kernel.h>
#include <scheduler.h>
#include <task.h>
#include <interrupts.h>
#include <syscall_kern.h>

#include <bwio.h>
#include <debug.h>
#include <term.h>
#include <track_node.h>
#include <string.h>
#include <track_data.h>

// Set by the context switch to determine whether the call is a system call
// or a context switch
extern volatile int __interrupt_type;

// tasks the kernel creates
void idle_task_entry();
void first_task_entry();

void printSensorTicks();

int main( int argc, char* argv[] ) {
	// setup COM2/terminal
	bwsetfifo( COM2, OFF );
	
	// setup peripherals and handlers
	kernel_init();

	// Create idle task	
	struct task* idle_task = task_create(&idle_task_entry, PRIORITY_IDLE, -1);
	scheduler_add(idle_task);

	// bootstrap first user task
	struct task* first_task = task_create(&first_task_entry, PRIORITY_HIGHEST, -1);
	scheduler_add(first_task);

	long idleTicks = 0;
	long userTicks = 0;	

	struct task *current_task;
	int request;
	long elapsed;
	
	while (1) {
		current_task = scheduler_get_next();
		if (current_task == 0) {
			break;
		}
		
ACTIVATE:

		// Time the execution of the task
		elapsed = rtc_get_ticks();

		// run the user task
		request = task_activate(current_task);

		elapsed = rtc_get_ticks() - elapsed;

		if (current_task == idle_task) {
			idleTicks += elapsed;
		}
		userTicks += elapsed;
		
		switch (__interrupt_type) {
			case INT_TYPE_HWI:
				// handle the interrupt request and determine if we should preempt the running task
				if (interrupts_handle_irq(current_task) == HWI_CONTINUE && current_task != idle_task) {
					goto ACTIVATE;
				} else {
					scheduler_add(current_task);
				}
				break;
			
			case INT_TYPE_SWI:
				// handle the system call request and determine if we should shutdown
				if (sys_handle_request(current_task, request) == -1) {
					goto shutdown;
				}
				break;
				
			default:
				KASSERT(0, "We got a weird interrupt type... %d", __interrupt_type);
				break;
		}
	}
	
shutdown:
	kernel_shutdown();
	
	// reset the scrolling window, in case it exists!
	bwprintf(COM2, CURSOR_SCROLL, 0, 999);
	bwprintf(COM2, CURSOR_CLEAR_SCREEN);

//	int i;
//	for (i = 0; i < MAX_NUM_TASKS; i++) {
//		task_print(COM2, i);
//	}

	printSensorTicks();

	bwprintf(COM2, "Idle time: %d%%\r\n", ((idleTicks * 100) / userTicks));
	bwprintf(COM2, "Kernel exiting.\r\n");
	return 0;
}

