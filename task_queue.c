#include <task_queue.h>
#include <task.h>

#include <bwio.h>
void task_queue_push_back(struct task_queue* queue, struct task* task) {
	if (queue->front == 0) {
		queue->front = task;
		queue->back = task;
		task->next = 0;
	} else {
		queue->back->next = task;
		queue->back = task;
		task->next = 0;
	}
}

struct task* task_queue_pop_front(struct task_queue* queue) {
	struct task* task = queue->front;
	if (queue->front != 0) {
		/* Move the front position to the second entry */
		queue->front = queue->front->next;
		/* If there is no second entry, set the back pointer to nothing too */
		if (queue->front == 0) {
			queue->back = 0;
		}
		task->next = 0;
	}
	return task;
}

struct task* task_queue_find(struct task_queue *queue, struct task *task) {
	struct task *i_task = queue->front;

	while (i_task != 0) {
		if (i_task == task) {
			i_task = i_task->next;
		}
	}

	return i_task;
}

void task_queue_remove(struct task_queue* queue, struct task *task) {

	struct task *prev_task = queue->front;
	struct task *cur_task = queue->front;

	while (cur_task != 0) {
		if (cur_task == task) {
			prev_task->next = cur_task->next;
			break;
		}
	}

	if (cur_task == queue->front) {
		queue->front = cur_task->next;
	}
	if (cur_task == queue->back) {
		queue->back = prev_task;
	}

}
