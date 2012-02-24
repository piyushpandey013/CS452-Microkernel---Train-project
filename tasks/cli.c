#include <io.h>
#include <syscall.h>


#include <name_server.h>
#include <uart_server.h>
#include <term.h>

void cli_task_entry() {

	RegisterAs("cli");
	int uart_tid = WhoIs(UART_SERVER);
	int term_tid = WhoIs(TERM_SERVER);
	int main_tid = MyParentTid(); // should be main_task!

	int reply = 1;

	char msg[100];
	while (1) {
		int len = 0;
		int c;
		
		int retval = TermPrint("> ", term_tid);
		AssertF(retval > 0, "Couldn't display prompt symbol. TermPrint returned %d\r", retval);
		while ((c = Getc(COM2, uart_tid)) != '\r') {
			// If we have room in the message, add characters
			if (c == '\b') {
				if (len > 0) {
					len--;
					TermPrint("\b \b", term_tid);
				}
			} else if (len < sizeof(msg) - 1) {
				msg[len++] = (char)c;

				// echo the character
				if (TermPutc((char)c, term_tid) < 0) {
					Printf("CLI is exiting\r");
					Exit(); // couldn't send to term, so it must be dead!
				}
			}	
		}
		
		if (TermPutc('\r', term_tid) < 0) {
			Printf("CLI is exiting\r");
			Exit();
		}
		
		// We got a new line, send the string contents to main task
		msg[len] = 0;
		if (Send(main_tid, (char*)msg, sizeof(msg), (char*)&reply, sizeof(reply)) < 0) {
			// could not send so main_task must've exited
			Exit();
		}
	}

	Exit();
}
