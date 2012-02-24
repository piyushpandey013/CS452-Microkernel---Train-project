#include <location.h>
#include <track_node.h>
#include <switch.h>

struct location* node_to_location(struct track_node* node, struct location* loc) {
	if (node->type == NODE_SENSOR) {
		loc->node = node->num;
	} else if (node->type == NODE_BRANCH) {
		if (node->num >= 153 && node->num <= 156) {
			loc->node = 116 + ((node->num - 153) * 2);
		} else {
			loc->node = 80 + ((node->num - 1) * 2);
		}
	} else if (node->type == NODE_MERGE) {
		if (node->num >= 153 && node->num <= 156) {
			loc->node = 117 + ((node->num - 153) * 2);
		} else {
			loc->node = 81 + ((node->num - 1) * 2);
		}
	}
	return loc;
}

inline struct track_node* location_to_node(struct track_node* track, struct location* loc) {
	return &track[loc->node];
}

// returns normalized node (the normalized node is not necessarily a sensor)
int normalize_location(struct track_node* track, char* switch_table, struct location* loc) {
	struct track_node* node = &track[loc->node];
	int offset = loc->offset;
	while (node->type != NODE_EXIT) {
		struct track_edge* edge = &node->edge[DIR_AHEAD];

		if (node->type == NODE_BRANCH) {
			int node_direction = (switch_table[node->num] == SWITCH_STRAIGHT ? DIR_STRAIGHT : DIR_CURVED);
			edge = &node->edge[node_direction];
		}

		if (offset < edge->dist) {
			node_to_location(node, loc);
			loc->offset = offset;
			return 0;
		}

		offset -= edge->dist;
		node = edge->dest;
	}
	return -1;
}
