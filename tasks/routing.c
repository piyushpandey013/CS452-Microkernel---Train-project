#include <heap.h>
#include <string.h>
#include <track_node.h>
#include <track_data.h>
#include <track_server.h>

#define REVERSE_WEIGHT 1000 // weight, in mm, for reversing

int dist_cmp(void *a, void *b) {
	struct track_node *aa = (struct track_node*)a;
	struct track_node *bb = (struct track_node*)b;

	return (aa->routing_info.distance > bb->routing_info.distance ? 1 : 0);
}

int reservation_edge_available(int train, struct track_edge *e);

void mark_path(int train, struct track_node *track, struct track_node *start, struct track_node *end) {

	// mark all track nodes for path finding
	int i = 0;
	for (i = 0; i < TRACK_MAX; ++i) {
		track[i].routing_info.distance = -1;
		track[i].routing_info.previous = 0;
		track[i].routing_info.visited = 0;
	}

	start->routing_info.distance = 0;

	struct heap h;
	struct track_node *nodes[TRACK_MAX];
	heap_init(&h, (void**)nodes, TRACK_MAX, &dist_cmp);

	heap_push(&h, start);

	while (!heap_empty(&h)) {
		struct track_node *v = (struct track_node *)heap_pop(&h);
		v->routing_info.visited = 1;
		//	  NODE_NONE,
		//	  NODE_SENSOR,
		//    NODE_BRANCH,
		//	  NODE_MERGE,
		//	  NODE_ENTER,
		//	  NODE_EXIT,

		if (v == end) {
			return;
		}

		if (v->type != NODE_EXIT) {
			// exit nodes don't have an edge ahead, so skip those

			struct track_node *adj_node = v->edge[DIR_AHEAD].dest;
			if ((!adj_node->routing_info.visited)
					&& (adj_node->routing_info.distance > v->edge[DIR_AHEAD].dist + v->routing_info.distance)
					&& (reservation_edge_available(train, &(v->edge[DIR_AHEAD])))) {

				adj_node->routing_info.distance = v->edge[DIR_AHEAD].dist + v->routing_info.distance;

				if (adj_node->routing_info.previous == 0) {
					heap_push(&h, adj_node);
				} else {
					heap_update(&h, adj_node);
				}
				adj_node->routing_info.previous = v;
			}
		}

		if (v->type == NODE_BRANCH) {

			// branches also have a CURVED edge, so go to those
			struct track_node *adj_node2 = v->edge[DIR_CURVED].dest;
			if ((!adj_node2->routing_info.visited)
					&& (adj_node2->routing_info.distance > v->edge[DIR_CURVED].dist + v->routing_info.distance)
					&& (reservation_edge_available(train, &(v->edge[DIR_CURVED])))
			) {

				adj_node2->routing_info.distance = v->edge[DIR_CURVED].dist + v->routing_info.distance;

				if (adj_node2->routing_info.previous == 0) {
					heap_push(&h, adj_node2);
				} else {
					heap_update(&h, adj_node2);
				}
				adj_node2->routing_info.previous = v;
			}
		}

		// now consider the reverse nodes
		struct track_node *rev_node = v->reverse;
		if ((!rev_node->routing_info.visited)
				&& (rev_node->routing_info.distance > REVERSE_WEIGHT + v->routing_info.distance)
				&& (reservation_edge_available(train, &(v->edge[DIR_CURVED])))
		) {

			rev_node->routing_info.distance = REVERSE_WEIGHT + v->routing_info.distance;

			if (rev_node->routing_info.previous == 0) {
				heap_push(&h, rev_node);
			} else {
				heap_update(&h, rev_node);
			}
			rev_node->routing_info.previous = v;
		}

	}

	start->routing_info.distance = -1;
	end->routing_info.visited = 0;
}
