#include <string.h> // MAX()
#include <track_util.h>
/*
     ----x--
    /
x---
    \----x--
*/

// assumes start->end is reachable.
int dist_between_adjacent_sensors(struct track_node * start, struct track_node * end, int distance) {

	if (start->type == NODE_SENSOR) {
		if (start == end) return distance;
		else return 0;
	}

	if (start->type == NODE_EXIT) return 0;

	if (start->type == NODE_BRANCH) {
		int a = dist_between_adjacent_sensors(start->edge[DIR_STRAIGHT].dest, end, distance + start->edge[DIR_STRAIGHT].dist);
		int b = dist_between_adjacent_sensors(start->edge[DIR_CURVED].dest, end, distance + start->edge[DIR_CURVED].dist);

		return MAX(a,b);
	}

	return dist_between_adjacent_sensors(start->edge[DIR_AHEAD].dest, end, distance + start->edge[DIR_AHEAD].dist);

}
