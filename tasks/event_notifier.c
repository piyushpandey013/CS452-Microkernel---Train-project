#include <event.h>
#include <syscall.h> // receive/reply

void event_notifier_entry() {
	int server, evt_type;
	int reply = 0;
	Receive(&server, (char*)&evt_type, sizeof(evt_type));
	Reply(server, (char*)&reply, sizeof(reply));

	if (evt_type < EVENT_START || evt_type > EVENT_END) {
		Exit();
	}

	// Tell the server we're ready. It is up to the server to decide when we should
	// start AwaitEvent-ing
	Send(server, (char*)&reply, sizeof(reply), (char*)&reply, sizeof(reply));

	while (1) {
		int data = AwaitEvent(evt_type);
		Send(server, (char*)&data, sizeof(data), (char*)&reply, sizeof(reply));
	}
}
