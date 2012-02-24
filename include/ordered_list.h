#ifndef __ORDERED_LIST_H__
#define __ORDERED_LIST_H__

struct ordered_list_node {
	struct ordered_list_node *next;
	int data;
};

struct ordered_list {
	struct ordered_list_node *front;
	struct ordered_list_node *back;
};

void ordered_list_add(struct ordered_list* l, struct ordered_list_node* node);
struct ordered_list_node* ordered_list_front(struct ordered_list *l);
struct ordered_list_node* ordered_list_pop_front(struct ordered_list* l);


#endif
