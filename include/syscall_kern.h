#ifndef __SYSCALL_KERN_H__
#define __SYSCALL_KERN_H__

#include <task.h>

int sys_create(struct task *t, int priority, void (*code)());
int sys_my_tid(struct task *t);
int sys_my_parent_tid(struct task *t);
void sys_pass(struct task *t);
void sys_exit(struct task *t);

int sys_send(struct task *t, int tid, char *msg, int msglen, char *reply, int replylen);
int sys_receive(struct task *t, int *tid, char *msg, int msglen);
int sys_reply(struct task *t, int tid, char *reply, int replylen);

int sys_await_event(struct task *t, int eventId);

int sys_assert(struct task* t, char* message);

int sys_handle_request(struct task* current_task, int request);

#endif
