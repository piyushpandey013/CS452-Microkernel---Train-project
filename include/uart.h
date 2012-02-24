#ifndef __UART_H__
#define __UART_H__

void uart_init(int port);
void uart_enable_int(int port);
void uart_disable_int(int port);
inline void uart_noops();
#endif

