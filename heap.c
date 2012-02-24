#include <string.h>
#include <heap.h>

#define LEFT(i) (2*(i) + 1)
#define RIGHT(i) (2*(i) + 2)
#define PARENT(i) (((i)-1)/2)

#define IS_LEAF(h, i) ((i) > ((h)->num_nodes / 2))
#define NOT_LEAF(h, i) (!IS_LEAF((h), (i)))

static void bubble_up(struct heap *h, int index) {
	int i = index;
	while ((PARENT(i) >= 0) && (h->fn(h->nodes[PARENT(i)], h->nodes[i]))) {
		// PARENT(i) > i. then,
		// swap PARENT(i) and i:
		void *temp = h->nodes[PARENT(i)];
		h->nodes[PARENT(i)] = h->nodes[i];
		h->nodes[i] = temp;
		
		i = PARENT(i);
	}
}

static void bubble_down(struct heap *h, int index) {
	int i = index;
	while (NOT_LEAF(h,i)) {
		int cmp = h->fn(h->nodes[LEFT(i)], h->nodes[RIGHT(i)]);
		
		int max_child;
		if (cmp) {
			// left > right
			max_child = RIGHT(i);
			if (max_child >= h->num_nodes) max_child = LEFT(i);
		} else {
			max_child = LEFT(i);
		}
		
		cmp = h->fn(h->nodes[i], h->nodes[max_child]);
		if (cmp) {
			// parent is greater than child!
			// swap them
			void *temp = h->nodes[max_child];
			h->nodes[max_child] = h->nodes[i];
			h->nodes[i] = temp;
			
			i = max_child;
		} else {
			break;
		}
	}
}

void heap_init(struct heap * h, void ** nodes, int size, heap_cmp fn) {
	h->nodes = nodes;
	h->node_size = size;
	h->num_nodes = 0;
	h->fn = fn;
}

int heap_push(struct heap *h, void * node) {
	if (h->num_nodes == h->node_size) {
		return -1; // heap is full
	}
	
	int i = h->num_nodes;
	h->nodes[i] = node;
	bubble_up(h, i); // sift-up's from index i
	
	h->num_nodes++;

	return 1;
}

int heap_empty(struct heap *h) {
	return (h->num_nodes == 0);
}

void * heap_pop(struct heap *h) {
	if (h->num_nodes == 1) {
		h->num_nodes--;
		return h->nodes[0];
	}
	
	// swap nodes[0] and nodes[num_nodes-1], and bubble down h[0]!
	void * temp = h->nodes[0];
	h->nodes[0] = h->nodes[h->num_nodes-1];
	h->num_nodes--;
	
	bubble_down(h, 0);
	
	return temp;
}

void heap_update(struct heap *h, void *val) {
	// find val:
	int i;
	for (i = 0; i < h->num_nodes; ++i) {
		if (h->nodes[i] == val) {
			// does this value need to be bubble'd up, or bubble'd down?
			if (IS_LEAF(h,i)) {
				bubble_up(h, i);
			} else {
//				int cmp = h->fn(h->nodes[i], h->nodes[LEFT(i)]);

				//THIS CAN BE OPTIMIZED!
				bubble_down(h, i);
				bubble_up(h, i);
			}
			break;
		}
	}
	if (i == h->num_nodes) {
		heap_push(h, val);
	}
}
