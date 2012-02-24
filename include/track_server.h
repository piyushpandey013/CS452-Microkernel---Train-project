#ifndef __TRACK_SERVER_H__
#define __TRACK_SERVER_H__

struct location;
struct track_edge;

#define TRACK_SERVER "track_server"

#define TRACK_TYPE_A 0
#define TRACK_TYPE_B 1
#define TRACK_TYPE TRACK_TYPE_A

// The reserved section of a track edge
struct reservation {
	struct reservation* next;
	struct track_edge* edge;
	int train; // train that reserved it
	int start; // start point (in mm) of the edge of the reservation
	int stop; // stop point (in mm) of the edge of the reservation
};

int TrackReserve(int train, struct location* loc, int length, int branch_safety, int tid);
int TrackReservationString(int train, char *buf, int len, int tid);
int TrackDistanceBetweenNodes(int node1, int node2, int tid);
int TrackDistanceUntilGoal(struct location *start, struct location *end, int distance, int tid);
int TrackNextSensor(int sensor, int *distance,  int tid);
int TrackNextSensorDirected(int sensor, int direction, int *distance,  int tid);
int TrackNormalize(struct location *loc, struct location *target_loc, int tid);
int TrackReverseNode(int node, int tid);
int TrackRoute(int train, int start_node, int end_node, short *path, int path_size, int tid);
int TrackSwitchDirection(int train, int switch_node, int next_node, int tid);

struct track_node *get_track();

#endif
