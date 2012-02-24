#ifndef __TASK_QUEUE_H__
#define __TASK_QUEUE_H__

struct task;
struct task_queue {
	struct task* front;
	struct task* back;
};

void task_queue_push_back(struct task_queue* queue, struct task* task);
struct task* task_queue_pop_front(struct task_queue* queue);

#endif
