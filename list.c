#include <list.h>

void list_push_back(struct list* l, struct list_node* node) {
	if (l->front == 0) {
		l->front = node;
		l->back = node;
		node->next = 0;
	} else {
		l->back->next = node;
		l->back = node;
		node->next = 0;
	}
}

struct list_node* list_pop_front(struct list* l) {
	struct list_node* front_node = l->front;
	if (l->front != 0) {
		/* Move the front position to the second entry */
		l->front = l->front->next;
		/* If there is no second entry, set the back pointer to nothing too */
		if (l->front == 0) {
			l->back = 0;
		}
		front_node->next = 0;
	}
	return front_node;
}
