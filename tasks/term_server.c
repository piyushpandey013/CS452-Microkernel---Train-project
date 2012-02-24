#include <io.h>
#include <syscall.h>

#include <uart_server.h>
#include <name_server.h>
#include <term.h>
#include <string.h>

#define TERM_SERVER_MAX_MSG_SIZE 1024

void term_server_entry() {
	RegisterAs(TERM_SERVER);
	int uart_tid = WhoIs(UART_SERVER);

	char msg[TERM_SERVER_MAX_MSG_SIZE];
	int ok = 1;
	while (1) {

		int rtid;
		int len = Receive(&rtid, msg, sizeof(msg));
		Reply(rtid, (char*)&ok, sizeof(ok));

		if (len == 1) {
			Putc(COM2, msg[0], uart_tid);
		} else {
			Putstr(COM2, msg, uart_tid);
		}

	}
}

int TermPrint(char* message, int tid) {
	int reply;
	int len = strlen(message) + 1;
	AssertF(len <= TERM_SERVER_MAX_MSG_SIZE, "Message to be printed is too long for term server");
	return Send(tid, message, len, (char*)&reply, sizeof(reply));
}

int TermPutc(char c, int tid) {
	int reply;
	return Send(tid, (char*)&c, sizeof(c), (char*)&reply, sizeof(reply));
}

void TermPrintf(int tid, char *fmt, ...) {

	char output[512];

	/* SAME AS sprintf */
    int length = 0;

    va_list va;

    va_start(va,fmt);
    sformat( &length, output, fmt, va );
    va_end(va);

    TermPrint(output, tid);
}

