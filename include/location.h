#ifndef __LOCATION_H__
#define __LOCATION_H__

struct track_node;

struct location {
	int node;
	int offset;
};

struct location* node_to_location(struct track_node* node, struct location* loc);
struct track_node* location_to_node(struct track_node* track, struct location* loc);
int normalize_location(struct track_node* track, char* switch_table, struct location* loc);

#endif
