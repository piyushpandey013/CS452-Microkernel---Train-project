#include <task.h>
#include <interrupts.h>
#include <scheduler.h>
#include <bwio.h>
#include <ts7200.h>
#include <uart.h>
#include <syscall_kern.h>

#define VIC1IntEnable ((volatile int*)0x800B0010)
#define VIC1IRQStatus ((volatile int*)0x800B0000)
#define VIC1IntEnClear ((volatile int*)0x800B0014)
#define VIC1Protection ((volatile int*)0x800B0020)

#define VIC2IntEnable ((volatile int*)0x800C0010)
#define VIC2IRQStatus ((volatile int*)0x800C0000)
#define VIC2IntEnClear ((volatile int*)0x800C0014)
#define VIC2Protection ((volatile int*)0x800C0020)

#define TC2IO (1 << 5)
#define UART1_RX (1 << 23)
#define UART1_TX (1 << 24)
#define UART1_ALL (1 << (52 - 32))

#define UART2_RX (1 << 25)
#define UART2_TX (1 << 26)
#define UART2_ALL (1 << (54 - 32))

#define TIMER2_CLEAR ((volatile int*)(TIMER2_BASE + CLR_OFFSET))

// hardware interrupt handler
extern void hwi();

// A small stack for the IRQ mode so we can push values onto it
// temporarily while context switching
static int irq_stack[4];

static int uart1_tx_flag;
static int uart1_cts_flag;

static int num_missed_com1_rx_interrupts;

int getNumMissedBytes() {
	return num_missed_com1_rx_interrupts;
}

void interrupts_init() {
	// set a small stack for the irq handler and disable interupts
	// in the handler

	int* q = irq_stack + 3;

	asm volatile ("label:");
	asm volatile ("ldr r7, %0" : : "m" ((int)q) : "r7");
	asm volatile ("msr CPSR_cx, #0xd2"); // IRQ
	asm volatile ("mov sp, r7");
	asm volatile ("ldr r7, [sp, #0]");
	asm volatile ("msr CPSR_cx, #0xd3"); // SVC

	// setup the hardware interrupt handler
	int *hwient = (int*)0x38;
	*hwient = (int)&hwi;
	
	// disable protection
	*VIC1Protection = 0;
	
	// Turn on timer interrupts
	*VIC1IntEnable |= TC2IO;

	// turn on combined interrupts for uart1 (use extra interrupts for flow control)
	*VIC2IntEnable |= UART1_ALL | UART2_ALL;

	uart1_tx_flag = 0;
	uart1_cts_flag = *UART1_FLAGS & CTS_MASK;
}

void interrupts_enable(int eventId) {
	switch (eventId) {
	case EVENT_TIMER:
		break;

	case EVENT_UART1_RX:
		break;

	case EVENT_UART1_TX:
		uart_noops();
		*UART1_CTLR |= TIEN_MASK | MSIEN_MASK; // turn on TX & modem interrupts
		break;

	case EVENT_UART2_RX:
		break;

	case EVENT_UART2_TX:
		uart_noops();
		*UART2_CTLR |= TIEN_MASK;
		break;
	}

}

void interrupts_disable(int eventId) {
	switch (eventId) {
	case EVENT_TIMER:
		break;

	case EVENT_UART1_RX:
		break;

	case EVENT_UART1_TX:
		uart_noops();
		*UART1_CTLR &= ~TIEN_MASK & ~MSIEN_MASK; // turn on TX & modem interrupts
		break;

	case EVENT_UART2_RX:
		break;

	case EVENT_UART2_TX:
		uart_noops();
		*UART2_CTLR &= ~TIEN_MASK;
		break;
	}

}

void interrupts_disable_all() {
	*VIC1Protection = 1;

	// disable uart1 interrupts
	*VIC1IntEnClear |= UART1_ALL | UART2_ALL;

	// disable timer interrupts
	*VIC1IntEnClear |= TC2IO;
}

static int notify_tasks(struct task* current_task, int eventId, int data, int* count) {
	int returnType = HWI_CONTINUE;
	struct task* t;
	int n = 0;
	while ((t = scheduler_get_task_blocked_on_event(eventId)) != 0) {
		// the task is popped off the queue during the above operation
		// set the state to ready and add the task back to the ready queue

		t->state = STATE_READY;
		scheduler_add(t);

		// set the return value for the task
		struct user_registers *registers = (struct user_registers*)(t->sp - sizeof(struct user_registers));
		registers->r0 = data;

		// determine if we should preempt
		if (t->priority > current_task->priority) {
			returnType = HWI_PREEMPT;
		}
		n++;
	}

	if (count != 0) {
		*count = n;
	}
	return returnType;
}

int interrupts_handle_irq(struct task *t) {
	int returnType = HWI_CONTINUE;
	if (*VIC1IRQStatus & TC2IO) {
		int preempt = notify_tasks(t, EVENT_TIMER, 10, 0);
		returnType = returnType || preempt;
		*TIMER2_CLEAR = 1;
	}

	if (*VIC2IRQStatus & UART2_ALL) {
		if (*UART2_INTERRUPTS & RIS_MASK && *UART2_FLAGS & RXFF_MASK) {	
			// reading clears the uart's RX interrupt
			char rx_byte = *UART2_DATA;
			if (rx_byte == 0x1b) {
				sys_assert(t, "Pressed ESC");
				return HWI_CONTINUE;
			}

			int preempt = notify_tasks(t, EVENT_UART2_RX, rx_byte, 0);
			returnType = returnType || preempt;
		}

		if (*UART2_INTERRUPTS & TIS_MASK && !(*UART2_FLAGS & TXFF_MASK)) {
			// Disable interrupts for transmit. Whoever picks up
			// this event should enable them again at their own discretion
			// clear interrupt
			int preempt = notify_tasks(t, EVENT_UART2_TX, 0, 0);
			returnType = returnType || preempt;
			
			uart_noops();
			*UART2_CTLR &= ~TIEN_MASK;
		}
	}

	if (*VIC2IRQStatus & UART1_ALL) {
		// handle RX interrupt
		if (*UART1_INTERRUPTS & RIS_MASK && *UART1_FLAGS & RXFF_MASK) {

			// reading clears the uart's RX interrupt
			char rx_byte = *UART1_DATA;
			int c;
			int preempt = notify_tasks(t, EVENT_UART1_RX, rx_byte, &c);
			if (c == 0) {
				num_missed_com1_rx_interrupts++;
			}
			returnType = returnType || preempt;
		}

		if (*UART1_INTERRUPTS & TIS_MASK && !(*UART1_FLAGS & TXFF_MASK)) {
			// Disable interrupts for transmit. Whoever picks up
			// this event should enable them again at their own discretion
			// clear interrupt
			uart_noops();
			*UART1_CTLR &= ~TIEN_MASK; // disable uart
			uart1_tx_flag = 1;
		}
		
		// modem CTS interrupt!
		if (*UART1_INTERRUPTS & MIS_MASK) {
			*UART1_INTERRUPTS = 0; // clear MIS interrupt

			if (*UART1_FLAGS & CTS_MASK) {
				uart1_cts_flag = 1;
			}
		}

		if (uart1_tx_flag && uart1_cts_flag) {
			uart1_tx_flag = 0;
			uart1_cts_flag = 0;
			
			// we may have processed Rx, so we need to take the "better" returntype (aka., preempt)
			int preempt = notify_tasks(t, EVENT_UART1_TX, 0, 0);
			returnType = returnType || preempt;
		}
	}

	return returnType;
}
