#include <io.h>
#include <string.h>

#include <uart_server.h>
#include <name_server.h>

// we need to use sformat the same way sprintf uses it to get Printf
void sformat ( int *length, char *str, char *fmt, va_list va );

void Printf(char *fmt, ...) {

	int utid = WhoIs(UART_SERVER);

	char output[256];

	/* SAME AS sprintf */
    int length = 0;

    va_list va;

    va_start(va,fmt);
    sformat( &length, output, fmt, va );
    va_end(va);

    Putstr(COM2, output, utid);

}
