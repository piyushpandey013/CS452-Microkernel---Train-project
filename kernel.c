#include <ts7200.h>
#include <kernel.h>
#include <task.h>
#include <scheduler.h>
#include <interrupts.h>

// hardware
#include <timer.h>
#include <uart.h>

#include <bwio.h>
#include <debug.h>

// software interrupt handler
extern void swi();

// This global is used to store whether the reason
// we entered the kernel is due to a software
// interrupt(syscall) or a hardware interrupt (event)
extern int __interrupt_type;

static void data_abort_handler() {
	int exception_addr;
	asm volatile ("str lr, %0" : : "m" (exception_addr) : "lr" ); // store the exception addr into variable
	exception_addr -= 4;
	
	bwprintf(COM2, "\rDATA ABORT HANDLER! exception at instruction %x\r", exception_addr);
	while(1);
}

static void prefetch_abort_handler() {
	int exception_addr;
	asm volatile ("str lr, %0" : : "m" (exception_addr) : "lr" ); // store the exception addr into variable
	exception_addr -= 4;
	
	bwprintf(COM2, "PREFETCH ABORT HANDLER! exception at instruction %x\r", exception_addr);
	while(1);
}

void kernel_init() {
	bwprintf(COM2, "Initializing kernel... ");

	// disable interrupts
	asm volatile ("msr CPSR_cx, #0xd3");

	// setup the system call handler
	int *kernent = (int*)0x28;
	*kernent = (int)&swi;
	
	// setup the abort exception handlers
	int *data_abort_ent = (int*)0x30;
	int *prefetch_abort_ent = (int*)0x2c;
	*data_abort_ent = (int)&data_abort_handler;
	*prefetch_abort_ent = (int)&prefetch_abort_handler;
	
	// enable watch dog
	//	*((volatile int*)(0x80940000)) = 0xAAAA;
	// reset watch dog timer
	//	*((volatile int*)(0x80940000)) = 0x5555;

	// device initialization
	timer2_init();
	timer3_init();

	uart_init(COM1);
	uart_init(COM2);
	rtc_init();

	// kernel service initialization
	task_init();
	scheduler_init();

	// interrupts initialization
	__interrupt_type = 0;
	interrupts_init();

	bwprintf(COM2, "OK\r\n");
}

void kernel_shutdown() {
	interrupts_disable_all();
	timer2_disable();
	timer3_disable();
	uart_disable_int(COM2);
	uart_disable_int(COM1);
}
