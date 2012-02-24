#include <ts7200.h>
#include <bwio.h>
#include <uart.h>

void uart_init(int port) {
	if (port == COM1) {
		bwsetspeed(COM1, 2400);

		 // turn off all interrupts!
		uart_disable_int(port);

		// Except reading
		uart_noops();
		*UART1_CTLR |= RIEN_MASK;

		uart_noops();
		*UART1_LINE |= STP2_MASK; // 2 stop bits
		
		uart_noops();
		*UART1_LINE &= ~PEN_MASK; // no parity

	} else {
		uart_noops();
		bwsetspeed(COM2, 115200);
		
		uart_disable_int(COM2);

		uart_noops();
		*UART2_CTLR |= RIEN_MASK;
	
		uart_noops();
		*UART2_LINE &= ~STP2_MASK & ~PEN_MASK; // 1 stop bits and no parity
	}

	uart_noops();
	bwsetfifo(port, OFF);
}

void uart_enable_int(int port) {
	uart_noops();
	if (port == COM1) {
		*UART1_CTLR |= MSIEN_MASK | TIEN_MASK | RIEN_MASK; // turn off modem interrupts
	} else {
		*UART2_CTLR |= TIEN_MASK | RIEN_MASK;
	}
}

void uart_disable_int(int port) {
	uart_noops();
	if (port == COM1) {
		*UART1_CTLR &= ~MSIEN_MASK & ~TIEN_MASK & ~RIEN_MASK & ~RTIEN_MASK; // turn off all interrupts
	} else {
		*UART2_CTLR &= ~TIEN_MASK & ~RIEN_MASK & ~MSIEN_MASK & ~RTIEN_MASK;;
	}
}

inline void uart_noops() {
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
}

