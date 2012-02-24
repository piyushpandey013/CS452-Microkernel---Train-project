#include <syscall.h>
#include <string.h>

/*
 * Task related system calls
 */
int MyTid() {
	
	int retval;

	// call mytid in the kernel
	asm volatile ("swi %0" : : "i" (SYS_MYTID));
	
	// get the return value (in r0), and store it into `retval`
	asm volatile ("str r0, %0" : : "m" (retval));
	
	return retval;
}

int Create(int priority, void (*code)()) {
	
	int retval;
	
	// call create in the kernel
	asm volatile ("swi %0" : : "i" (SYS_CREATE));
	// get the return value (in r0), and store it into `retval`
	asm volatile ("str r0, %0" : : "m" (retval));
	
	return retval;
}

int MyParentTid() {
	int retval;
	
	// call myparenttid in the kernel
	asm volatile ("swi %0" : : "i" (SYS_MYPARENTTID));
	// get the return value (in r0), and store it into `retval`
	asm volatile ("str r0, %0" : : "m" (retval));
	
	return retval;
}

void Pass() {
	asm volatile ("swi %0" : : "i" (SYS_PASS));
}

void Exit() {
	asm volatile ("swi %0" : : "i" (SYS_EXIT));
}

/*
 * Message passing related system calls
 */

int Send( int tid, char *msg, int msglen, char *reply, int replylen ) {

//	int arg0 = tid;
//	char *arg1 = msg;
//	int arg2 = msglen;
//	char* arg3 = reply;
//	int arg4 = replylen;
//
//	asm volatile ("stmdb sp!, {r0-r4}");
//	AssertF(arg4 > 0, "Send(): replylen wasn't > 0! replylen = %d", arg4	);
//	AssertF(arg0 > 0, "Sent(): tid wasn't > 0! tid = %d", arg0 );
//	asm volatile ("ldmia sp!, {r0-r4}");

	int retval;

	asm volatile ("stmdb sp!, {r4}");
	asm volatile ("ldr r4, %0" : : "m" (replylen));
	asm volatile ("swi %0" : : "i" (SYS_SEND));
	asm volatile ("str r0, %0" : : "m" (retval));
	asm volatile ("ldmia sp!, {r4}");

	return retval;
}

int Receive( int *tid, char *msg, int msglen ) {
	int retval;

	asm volatile ("swi %0" : : "i" (SYS_RECEIVE));
	asm volatile ("str r0, %0" : : "m" (retval));

	return retval;
}

int Reply( int tid, char *reply, int replylen ) {
	int retval;

	asm volatile ("swi %0" : : "i" (SYS_REPLY));
	asm volatile ("str r0, %0" : : "m" (retval));
	
	return retval;
}

int AwaitEvent( int eventId ) {
	int retval;

	asm volatile ("swi %0" : : "i" (SYS_AWAIT_EVENT));
	asm volatile ("str r0, %0" : : "m" (retval));

	return retval;
}

void Shutdown() {
	asm volatile ("swi %0" : : "i" (SYS_SHUTDOWN));
}

int Assert(char* message) {
	int retval;

	asm volatile ("swi %0" : : "i" (SYS_ASSERT));
	asm volatile ("str r0, %0" : : "m" (retval));

	return retval;
	
}

int AssertF(unsigned int condition, char* fmt, ...) {
	if (condition == 0) {
		// Assertion failed
		char message[100];
		int length = 0;

	    va_list va;
		va_start(va, fmt);
	    sformat(&length, message, fmt, va);
	    va_end(va);
		
		return Assert(message);
	}
	return 0;
}
