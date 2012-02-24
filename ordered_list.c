#include <ordered_list.h>
#include <list.h>

void ordered_list_add(struct ordered_list* l, struct ordered_list_node* node) {
	if (l->front == 0) {
		l->front = node;
		l->back = node;
		node->next = 0;
	} else {

		struct ordered_list_node *prev = 0;
		struct ordered_list_node *cur = l->front;

		while (cur != 0 && cur->data < node->data) {
			prev = cur;
			cur = cur->next;
		}

		if (prev == 0) {
			// we need to insert this node at the very front
			l->front = node;
			node->next = cur;
		} else if (cur == 0) {
			// we need to insert this node at the very back
			node->next = 0;
			l->back = node;
			prev->next = node;
		} else {
			// we insert it in the middle!
			prev->next = node;
			node->next = cur;
		}

	}
}

struct ordered_list_node* ordered_list_front(struct ordered_list *l) {
	return l->front;
}

struct ordered_list_node* ordered_list_pop_front(struct ordered_list* l) {
	return (struct ordered_list_node*)list_pop_front((struct list*)l);
}
