#ifndef __UART_SERVER_H__
#define __UART_SERVER_H__

#define UART_SERVER "uart_server"

struct uart_msg {
	int operation;
	int com;
	int data;
};

int Getc(int channel, int tid);
int Putc(int channel, int ch, int tid);
int Putstr(int channel, char *s, int tid);


#endif
