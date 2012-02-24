#ifndef __HEAP_H__
#define __HEAP_H__

typedef int (*heap_cmp)(void *, void*);

struct heap {
	void **nodes; // array of items
	int node_size; // number of available nodes
	int num_nodes; // length of heap so far

	heap_cmp fn; // function that tells us > or <= for two nodes
	// fn(a,b) = 1 if a > b, 0 otherwise.
};

void heap_init(struct heap * h, void ** nodes, int size, heap_cmp fn);
void * heap_pop(struct heap *h);
int heap_push(struct heap *h, void * node);
void heap_update(struct heap *h, void *val);
int heap_empty(struct heap *h);

#endif
