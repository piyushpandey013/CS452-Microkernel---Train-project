#include <task.h>
#include <io.h>
#include <event.h>
#include <ts7200.h>

#include <list.h>
#include <cbuf.h>
#include <clist.h>
#include <string.h>
#include <io.h>
#include <uart.h>

#include <uart_server.h>
#include <name_server.h>
#include <syscall.h>

#define GETC 0
#define PUTC 1
#define PUTSTR 2

#define COM1 0
#define COM2 1

#define MAX_GETC_QUEUE_SIZE MAX_NUM_TASKS
#define MAX_PUTC_QUEUE_SIZE MAX_NUM_TASKS

#define MAX_TRANSMIT_BUFFER_SIZE 1024

void extern event_notifier_entry();

struct putc_node {
	struct list_node parent;
	int tid;
	int data;
};

static int getc_wait_buffer[2][MAX_GETC_QUEUE_SIZE];
static struct putc_node putc_wait_buffer[2][MAX_PUTC_QUEUE_SIZE];

static struct clist getc_wait_list[2];
static struct list putc_wait_list[2];
static struct list putc_free_list[2];

// create transmit circular buffers for both UARTs
static char uart_tx_buffer[2][MAX_TRANSMIT_BUFFER_SIZE];

// create circular buffers for transmission from both UARTs
static struct cbuf uart_tx_cb[2];

int getBufSize() {
	return cbuf_len(&uart_tx_cb[COM2]);
}

void uart_server_entry() {

	// initialize the circular buffers holding the transmit characters for UART1 and UART2
	cbuf_init(&uart_tx_cb[COM1], uart_tx_buffer[COM1], sizeof(uart_tx_buffer[COM1]));
	cbuf_init(&uart_tx_cb[COM2], uart_tx_buffer[COM2], sizeof(uart_tx_buffer[COM2]));

	// initialize queues to hold tasks waiting for characters (for Getc())
	clist_init(&getc_wait_list[COM1], getc_wait_buffer[COM1], MAX_GETC_QUEUE_SIZE);
	clist_init(&getc_wait_list[COM2], getc_wait_buffer[COM2], MAX_GETC_QUEUE_SIZE);
	
	// Initialize putc wait list
	int i;
	for (i = 0; i < MAX_PUTC_QUEUE_SIZE - 1; i++) {
		putc_wait_buffer[COM1][i].parent.next = (struct list_node*)&putc_wait_buffer[COM1][i + 1];
		putc_wait_buffer[COM2][i].parent.next = (struct list_node*)&putc_wait_buffer[COM2][i + 1];
	}
	putc_wait_buffer[COM1][i].parent.next = 0;
	putc_wait_buffer[COM2][i].parent.next = 0;
	putc_free_list[COM1].front = putc_free_list[COM1].back = (struct list_node*)&putc_wait_buffer[COM1];
	putc_free_list[COM2].front = putc_free_list[COM2].back = (struct list_node*)&putc_wait_buffer[COM2];
	putc_wait_list[COM1].front = putc_wait_list[COM1].back = 0;
	putc_wait_list[COM2].front = putc_wait_list[COM2].back = 0;
	
	// create notifiers for receiving and transmitting events for both UARTs
	int rx_notifier_tid[2];
	int tx_notifier_tid[2];

	int event_type;
	
	// receive notifier for UART1
	rx_notifier_tid[COM1] = Create(PRIORITY_HIGHEST, &event_notifier_entry);
	event_type = EVENT_UART1_RX;
	Send(rx_notifier_tid[COM1], (char*)&event_type, sizeof(event_type), (char*)&event_type, sizeof(event_type));

	// transmit notifier for UART1
	tx_notifier_tid[COM1] = Create(PRIORITY_HIGHEST, &event_notifier_entry);
	event_type = EVENT_UART1_TX;
	Send(tx_notifier_tid[COM1], (char*)&event_type, sizeof(event_type), (char*)&event_type, sizeof(event_type));

	// receive notifier for UART2
	rx_notifier_tid[COM2] = Create(PRIORITY_HIGHEST, &event_notifier_entry);
	event_type = EVENT_UART2_RX;
	Send(rx_notifier_tid[COM2], (char*)&event_type, sizeof(event_type), (char*)&event_type, sizeof(event_type));

	// transmit notifier for UART2COM2
	tx_notifier_tid[COM2] = Create(PRIORITY_HIGHEST, &event_notifier_entry);
	event_type = EVENT_UART2_TX;
	Send(tx_notifier_tid[COM2], (char*)&event_type, sizeof(event_type), (char*)&event_type, sizeof(event_type));
	
	
	int sender_tid;
	// Wait for all 4 notifiers to report
	// start them up and have them wait for events
	int start_ok;
	int c = 0;
	while (c < 4) {
		Receive(&sender_tid, (char*)&start_ok, sizeof(start_ok));
		if (sender_tid == tx_notifier_tid[COM1] || sender_tid == rx_notifier_tid[COM1] ||
				sender_tid == tx_notifier_tid[COM2] || sender_tid == rx_notifier_tid[COM2]) {
			c++;
		} else {
			int bad = -1;
			Reply(sender_tid, (char*)&bad, sizeof(bad));
			Debug("uart_server: got a non-notifier message (from tid=%d) during uart_server startup. THIS IS BAD!\r", sender_tid);
		}
	}


	RegisterAs("uart_server");
	struct uart_msg msg;

	// 1 if notifier is AwaitEvent()ing, 0 otherwise
	int tx_notifier_awaiting[2] = {0, 0}; /* for com1 and com2 */
	while (1) {
		Receive(&sender_tid, (char*)&msg, sizeof(msg));
		
		if (sender_tid == rx_notifier_tid[COM1] || sender_tid == rx_notifier_tid[COM2]) {
			/*
			 * Handle Receive Interrupt
			 * the message 'msg' should be interpretted as an int
			 */
			int channel = COM1;
			if (sender_tid == rx_notifier_tid[COM2]) {
				channel = COM2;
			}

			// if there's a process waiting for a char, it should get it!
			int waiting_tid;
			while (clist_pop(&getc_wait_list[channel], &waiting_tid) >= 0) {
				Reply(waiting_tid, (char*)&msg, sizeof(int));
			}
		} else if (sender_tid == tx_notifier_tid[COM1] || sender_tid == tx_notifier_tid[COM2]) {
			/*
			 * Handle Tx interrupt
			 * the message is an int
			 */
			int reply = 0;
			
			int channel = COM1;
			if (sender_tid == tx_notifier_tid[COM2]) {
				channel = COM2;
			}

			tx_notifier_awaiting[channel] = 0;

			// if there's a byte to write, write it
			char ch;
			if (cbuf_read(&uart_tx_cb[channel], &ch, sizeof(ch))) {
				if (channel == COM1) {
					*UART1_DATA = (int)ch;
				} else {
					*UART2_DATA = (int)ch;
				}
				
				if (cbuf_free(&uart_tx_cb[channel]) > 0 && putc_wait_list[channel].front != 0) {
					// Our buffer has space and tasks are waiting to PUTC
					struct putc_node* node = (struct putc_node*)list_pop_front(&putc_wait_list[channel]);
					char c = (char)node->data;
					AssertF(cbuf_write(&uart_tx_cb[msg.com], (char*)&c, sizeof(c)) == 1, "Could not write to buffer from wait list");
					Reply(node->tid, (char*)&reply, sizeof(reply));
					list_push_back(&putc_free_list[channel], (struct list_node*)node);
				}
			}

			// if there are more bytes, tell the notifier to enable uart interrupts
			reply = 0;
			if (cbuf_len(&uart_tx_cb[channel]) > 0) {
				tx_notifier_awaiting[channel] = 1;
				Reply(tx_notifier_tid[channel], (char*)&reply, sizeof(reply));
			} else {
				tx_notifier_awaiting[channel] = 0;
			}
		} else {
			/*
			 * Handle msgs from Getc/Putc
			 * the message is a uart_msg
			 */
			int reply = 0;	
			if (msg.com == COM1 || msg.com == COM2) {
				char ch;
				
				switch (msg.operation) {
					case GETC:
						// Add the task to the waiting list
						clist_push(&getc_wait_list[msg.com], sender_tid);
						
						if (clist_len(&getc_wait_list[msg.com]) == 1) {
							// Reply to the notifier so it can continue waiting for events
							Reply(rx_notifier_tid[msg.com], (char*)&reply, sizeof(reply));
						}
						break;

					case PUTC:
						ch = (char)msg.data;
						reply = -3;
						if (cbuf_write(&uart_tx_cb[msg.com], &ch, sizeof(ch)) == 1) {
							// since we have something in the buffer to write, we need to wake up the notifier if it's sleeping!
							if (!tx_notifier_awaiting[msg.com]) {
								tx_notifier_awaiting[msg.com] = 1;
								Reply(tx_notifier_tid[msg.com], (char*)&reply, sizeof(reply));
							}

							reply = 0; // success
							Reply(sender_tid, (char*)&reply, sizeof(reply));
						} else {
							AssertF(putc_free_list[msg.com].front != 0, "PUTC wait list full for COM %d", msg.com + 1);
							struct putc_node* node = (struct putc_node*)list_pop_front(&putc_free_list[msg.com]);
							node->tid = sender_tid;
							node->data = msg.data;
							list_push_back(&putc_wait_list[msg.com], (struct list_node*)node);
						}
						break;

					default:
						// Illegal operation specified
						AssertF(0, "Illegal operation specified to uart_server operation=%d from sender_tid=%d", msg.operation, sender_tid);
						reply = -1;
						Reply(sender_tid, (char*)&reply, sizeof(reply));
						break;
				}

			} else {
				// Incorrect port/channel specified
				AssertF(0, "Invalid com port specified to uart_server. com=%d, sender_tid=%d", msg.com, sender_tid);
				reply = -4;
				Reply(sender_tid, (char*)&reply, sizeof(reply));
			}
		}
	}

}

int Getc(int channel, int tid) {
	int reply;
	struct uart_msg msg;
	msg.com = channel;
	msg.operation = GETC;

	Send(tid, (char*)&msg, sizeof(msg), (char*)&reply, sizeof(reply));

	return reply;
}

int Putc(int channel, int ch, int tid) {
	int reply;
	struct uart_msg msg;
	msg.com = channel;
	msg.operation = PUTC;
	msg.data = ch;

	if ((msg.com != 0) && (msg.com != 1)) {
		AssertF(0, " Putc(): msg.com=%d is an invalid COM", msg.com);
	}
	Send(tid, (char*)&msg, sizeof(msg), (char*)&reply, sizeof(reply));

	return reply;
}

int Putstr(int channel, char *s, int tid) {
	while (*s) {
		if (Putc(channel, (int)*s, tid) < 0) {
			return -1;
		}
		s++;
	}

	return 0;
}
