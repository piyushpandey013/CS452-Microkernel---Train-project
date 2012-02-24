#ifndef __LIST_H__
#define __LIST_H__

struct list_node {
	struct list_node *next;
};

struct list {
	struct list_node *front;
	struct list_node *back;
};


struct list_node * list_pop_front(struct list* l);
void list_push_back(struct list* l, struct list_node* node);
#endif
