#ifndef __SYSCALL_H__
#define __SYSCALL_H__

#include <scheduler.h> // for the PRIORITY_*

#define SYS_CREATE 1
#define SYS_MYTID 2
#define SYS_MYPARENTTID 3
#define SYS_PASS 4
#define SYS_EXIT 5
#define SYS_DESTROY 6
#define SYS_SEND 7
#define SYS_RECEIVE 8
#define SYS_REPLY 9
#define SYS_TICKS 10
#define SYS_AWAIT_EVENT 11
#define SYS_ASSERT 12
#define SYS_SHUTDOWN 13

int Create(int priority, void (*code)());
int MyTid();
int MyParentTid();
void Pass();
void Exit();
void Shutdown();

int Send( int tid, char *msg, int msglen, char *reply, int replylen );
int Receive( int *tid, char *msg, int msglen );
int Reply( int tid, char *reply, int replylen );

int AwaitEvent( int eventId );
int Assert(char* message);
int AssertF(unsigned int condition, char* fmt, ...);

#endif
