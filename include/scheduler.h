#ifndef __SCHEDULER_H__
#define __SCHEDULER_H__

#define PRIORITY_HIGHEST 9
#define PRIORITY_NORMAL 4
#define PRIORITY_LOWEST 1
#define PRIORITY_IDLE 0

void scheduler_init();
struct task * scheduler_get_next();
struct task * scheduler_get_task_blocked_on_event(int eventId);
int scheduler_add(struct task *t);
void scheduler_set_idle_task(struct task *t);

void scheduler_print_queues(int port);
void scheduler_print_event_queue(int port, int eventId);

#endif
