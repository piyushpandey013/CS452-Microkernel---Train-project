#include <syscall.h>
#include <name_server.h>
#include <track_server.h>

#include <track_data.h>
#include <track_node.h>
#include <location.h>
#include <switch.h>

#include <string.h>
#include <term.h>

#include <routing.h>

#define MAX_RESERVATION_LENGTH	30000
#define MAX_TRAIN_RESERVATIONS 4
#define SEGMENTS_PER_TRAIN 44	// so many so that occupied data can be duplicated on reverse edges

#define TRACK_DISPLAY_SCALE 0.05

// API request types
#define TRACK_OP_RESERVATION 0
#define TRACK_OP_TRACK_DISTANCE 1
#define TRACK_OP_TRACK_NEXT_SENSOR 2
#define TRACK_OP_TRACK_REVERSE_NODE 3
#define TRACK_OP_ROUTE 4
#define TRACK_OP_SWITCH_DIRECTION 5

#define TRACK_OP_NODE_IS_RESERVED 6
#define TRACK_OP_TRACK_DISTANCE_UNTIL_GOAL 7

#define TRACK_OP_RESERVATION_STRING 8
#define TRACK_OP_NORMALIZE 9

// Message type sent by API methods
struct reservation_request_message {
	int operation;
	int train;
	struct location position;
	int length;
	int branch_safety; // this is pretty much the train length
};

struct track_request_message {
	int operation; // TRACK_OP_DISTANCE, TRACK_OP_NEXT_SENSOR, TRACK_
	int train;

	int node1;
	int offset1;

	int node2;
	int offset2;

	int distance;
};

struct track_reply_message {
	int node;
	int distance;
};

// A trains collection of reserved track edge sections
struct reservation_node {
	int train;
	struct reservation reservations[SEGMENTS_PER_TRAIN];
};

// Global statics
static struct track_node track[TRACK_MAX];
static char switch_table[256];
static struct reservation_node train_reservations[MAX_TRAIN_RESERVATIONS];

struct track_node *get_track() {
	return track;
}

// Initialize the reservation data structures
static void reservation_init(struct reservation_node* train_reservations) {
	int i;
	int train_index = 0;
	train_reservations[train_index].train = 41;
	for (i = 0; i < SEGMENTS_PER_TRAIN; i++) {
		train_reservations[train_index].reservations[i].train = train_reservations[train_index].train;
		train_reservations[train_index].reservations[i].edge = 0;
		train_reservations[train_index].reservations[i].next = 0;
	}
	train_index++;

	train_reservations[train_index].train = 35;
	for (i = 0; i < SEGMENTS_PER_TRAIN; i++) {
		train_reservations[train_index].reservations[i].train = train_reservations[train_index].train;
		train_reservations[train_index].reservations[i].edge = 0;
		train_reservations[train_index].reservations[i].next = 0;
	}
	train_index++;

	train_reservations[train_index].train = 36;
	for (i = 0; i < SEGMENTS_PER_TRAIN; i++) {
		train_reservations[train_index].reservations[i].train = train_reservations[train_index].train;
		train_reservations[train_index].reservations[i].edge = 0;
		train_reservations[train_index].reservations[i].next = 0;
	}
	train_index++;
}

// Map train numbers to reservations
static struct reservation_node* get_reservation(struct reservation_node* reservations, int train) {
	switch(train) {
		case 41:
			return &reservations[0];
		case 35:
			return &reservations[1];
		case 36:
			return &reservations[2];
		default:
			return 0;
	}
}

// Removes any reservations owned by the given train
static void clear_train_reservations(struct reservation_node* node) {
	int i;
	for (i = 0; i < SEGMENTS_PER_TRAIN; i++) {
		struct track_edge* edge = node->reservations[i].edge;
		if (edge != 0) {
			struct reservation* r = edge->reservation_node;
			struct reservation* p = 0;
			while (r != 0) {
				if (r == &node->reservations[i]) {
					if (p == 0) {
						edge->reservation_node = r->next;
						r->next = 0;
						r->edge = 0;
						break;
					} else {
						p->next = r->next;
						r->next = 0;
						r->edge = 0;
						break;
					}
				}
				p = r;
				r = r->next;
			}
		}
	}
}

static int reservation_available(int train, struct track_edge* edge, int startOffset, int stopOffset) {
	// Check if this segment of track is occupied y a train
	AssertF(edge != 0, "res_avail: EDGE PASSED IN IS NULL!");
	struct reservation* r = edge->reservation_node;
	while (r != 0) {
		if (r->train != train &&
				((startOffset >= r->start && startOffset <= r->stop) ||
				(stopOffset >= r->start && stopOffset <= r->stop) ||
				(startOffset <= r->start && stopOffset >= r->stop))) {
			// Now we are really crossing the line. Report that
			// reservation failed!
			return 0;
		}
		r = r->next;
	}
	return 1;
}

int reservation_edge_available(int train, struct track_edge* edge) {
	AssertF(edge != 0, "res_edge_avail: EDGE PASSED IN IS NULL!");
	return reservation_available(train, edge, 0, edge->dist);
}

// Verifies that the given reservation does not collide with existing reservations
// returns -1 if can't reserve. 0 otherwise
static int reservation_verify(int train, struct location* position, int length, int branch_safety) {
	// For each segment of track occupied by the new reservation
	// check if any other train is occupying it

	int node_direction;
	struct track_node* node = location_to_node(track, position);
	if (node->type == NODE_BRANCH) {
		node_direction = switch_table[node->num] == SWITCH_STRAIGHT ? DIR_STRAIGHT : DIR_CURVED;
	} else {
		node_direction = DIR_AHEAD;
	}

	struct track_edge* occupied_edge = &(node->edge[node_direction]);

	int startOffset = position->offset;
	int total_length = 0;

	while (length > 0) {
		int stopOffset = MIN(length + startOffset, occupied_edge->dist);

		if (occupied_edge->src->type == NODE_BRANCH) {
			// The is an edge coming from a branch, so we must make sure we cover the whole
			// length of it so we don't get stuck on the switch
			stopOffset = occupied_edge->dist;
		}

		// Check if this segment of track is occupied by a train
		if (reservation_available(train, occupied_edge, startOffset, stopOffset) == 0) {
			// This segment is taken
			return -1;
		}

		length -= stopOffset - startOffset;
		total_length += stopOffset - startOffset;

		startOffset = 0;

		// if this is a branch, make sure we reserve branch_safety more afterwards!
		if (occupied_edge->src->type == NODE_BRANCH) {
			length = MAX(length, branch_safety);
		}

		// on to the next edge!
		if (occupied_edge->dest->type == NODE_BRANCH) {
			node_direction = switch_table[occupied_edge->dest->num] == SWITCH_STRAIGHT ? DIR_STRAIGHT : DIR_CURVED;
		} else if (occupied_edge->dest->type == NODE_EXIT && length != 0) {
			return -1;
		} else {
			node_direction = DIR_AHEAD;
		}

		if (occupied_edge->dest->type != NODE_EXIT) {
			occupied_edge = &occupied_edge->dest->edge[node_direction];
		} else if (length > 0) {
			return -3; // we hit an exit!
		}
	}

	if (total_length >= MAX_RESERVATION_LENGTH) {
		Printf("Train %d reservation was too big (%d mm)!\r", train, total_length);
		return -2;
	}
	return 0;
}

// Inserts the reservation into the required track segments
static void reservation_insert(struct reservation_node* reservation, struct location* position, int length, int branch_safety) {
	int node_direction;
	struct track_node* node = location_to_node(track, position);
	if (node->type == NODE_BRANCH) {
		node_direction = switch_table[node->num] == SWITCH_STRAIGHT ? DIR_STRAIGHT : DIR_CURVED;
	} else {
		node_direction = DIR_AHEAD;
	}

	struct track_edge* occupied_edge = &(node->edge[node_direction]);
	int startOffset = position->offset;
	int r_index = 0;
	while (length > 0) {
		int stopOffset = MIN(length + startOffset, occupied_edge->dist);

		if (occupied_edge->src->type == NODE_BRANCH) {
			// The is an edge coming from a branch, so we must make sure we cover the whole
			// length of it so we don't get stuck on the switch
			stopOffset = occupied_edge->dist;
		}

		AssertF(r_index < SEGMENTS_PER_TRAIN, "Too many segments for train reservation");
		// Set the forward direction edge's occupied information
		reservation->reservations[r_index].start = startOffset;
		reservation->reservations[r_index].stop = stopOffset;
		reservation->reservations[r_index].edge = occupied_edge;
		reservation->reservations[r_index].next = occupied_edge->reservation_node;
		occupied_edge->reservation_node = &reservation->reservations[r_index];
		r_index++;

		// Set the reverse direction edge's occupied information
		AssertF(r_index < SEGMENTS_PER_TRAIN, "Too many segments for train reservation");
		reservation->reservations[r_index].start = occupied_edge->dist - stopOffset;
		reservation->reservations[r_index].stop = occupied_edge->dist - startOffset;
		reservation->reservations[r_index].edge = occupied_edge->reverse;
		reservation->reservations[r_index].next = occupied_edge->reverse->reservation_node;
		occupied_edge->reverse->reservation_node = &reservation->reservations[r_index];
		r_index++;


		length = length - (stopOffset - startOffset);

		// If this is a branch and the reservation ends somewhere in the next edge, make
		// sure we take branch safety into consideration
		if (occupied_edge->src->type == NODE_BRANCH) {
			length = MAX(length, branch_safety);
		}

		if (occupied_edge->dest->type == NODE_BRANCH) {
			node_direction = switch_table[occupied_edge->dest->num] == SWITCH_STRAIGHT ? DIR_STRAIGHT : DIR_CURVED;
		} else {
			node_direction = DIR_AHEAD;
		}

		occupied_edge = &occupied_edge->dest->edge[node_direction];
		startOffset = 0;
	}
}

static int node_is_reserved(int train, int node) {

	struct reservation *r = track[node].edge[DIR_STRAIGHT].reservation_node;
	while (r != 0) {
		if (r->train == train) {
			return 1; // reserved!
		}
		r = r->next;
	}

	if (track[node].type == NODE_BRANCH) {
		r = track[node].edge[DIR_CURVED].reservation_node;
		while (r != 0) {
			if (r->train == train) {
				return 1; // reserved!
			}
			r = r->next;
		}
	}

	return 0; // not reserved!
}

// Displays a graphical representation of the reservations
static int reservation_print(struct reservation_node* reservation, char *buf) {
	int i;
	char *p = buf;

	// Iterate by 2's since every other reservation is for the reverse case
	for (i = 0; i < SEGMENTS_PER_TRAIN; i += 2) {
		if (reservation->reservations[i].edge == 0) {
			break;
		}

		p += sprintf(p, "%s", reservation->reservations[i].edge->src->name);
		int s = reservation->reservations[i].start * TRACK_DISPLAY_SCALE;
		for (; s > 0; s--) {
			p += sprintf(p, "-");
		}

		s = (reservation->reservations[i].stop - reservation->reservations[i].start) * TRACK_DISPLAY_SCALE;
		for (; s > 0; s--) {
			p += sprintf(p, "=");
		}

		s = (reservation->reservations[i].edge->dist - reservation->reservations[i].stop) * TRACK_DISPLAY_SCALE;
		for (; s > 0; s--) {
			p += sprintf(p, "-");
		}
	}
	return (int)p-(int)buf + 1; // +1 for null termination
}

// Initialize track data structures
static void track_init() {
#if TRACK_TYPE == TRACK_TYPE_A
	init_tracka(track);
#else
	init_trackb(track);
#endif

	// Initialize the switch table
	int switch_tid = WhoIs(SWITCH_SERVER);
	int i;
	for (i = 1; i <= 18; i++) {

		switch_table[i] = (char)SwitchGetDirection(i, switch_tid);
	}

	switch_table[0x99] = (char)SwitchGetDirection(0x99, switch_tid);
	switch_table[0x9A] = (char)SwitchGetDirection(0x9A, switch_tid);
	switch_table[0x9B] = (char)SwitchGetDirection(0x9B, switch_tid);
	switch_table[0x9C] = (char)SwitchGetDirection(0x9C, switch_tid);
}

int dist_between_nodes(int start_num, int end_num) {
	struct track_node *start = &track[start_num];
	struct track_node *end = &track[end_num];

	AssertF(track != 0 && start != 0 && end != 0, "BAD INPUT TO dist_betwen_nodes", 0);

	int distance = 0;
	struct track_node *current = start;

	while (current->num != end->num || current->type != end->type) {

		if (current->type == NODE_EXIT) {
			return 0;
		} else if (current->type == NODE_BRANCH) {
			int switch_direction = switch_table[current->num];
			int node_direction = (switch_direction == SWITCH_STRAIGHT ? DIR_STRAIGHT : DIR_CURVED);

			distance += current->edge[node_direction].dist;
			current = current->edge[node_direction].dest;
		} else {
			distance += current->edge[DIR_AHEAD].dist;
			current = current->edge[DIR_AHEAD].dest;
		}

		if (current == start) {
			// HORRY SHIITTu, WE ARE IN A ROOP!
			return -1;
		}

	}

	return distance;
}


static int dist_until_goal(int start_num, int start_offset, int end_num, int end_offset, int max_distance) {
	struct track_node *start = &track[start_num];
	struct track_node *end = &track[end_num];

	AssertF(track != 0 && start != 0 && end != 0, "BAD INPUT TO dist_until_goal", 0);

	int distance = 0 - start_offset;
	struct track_node *current = start;

	while (!(current->num == end->num && current->type == end->type)) {

		if (current->type == NODE_EXIT) {
			return 0;
		} else if (current->type == NODE_BRANCH) {
			int switch_direction = switch_table[current->num];
			int node_direction = (switch_direction == SWITCH_STRAIGHT ? DIR_STRAIGHT : DIR_CURVED);

			distance += current->edge[node_direction].dist;
			current = current->edge[node_direction].dest;
		} else {
			distance += current->edge[DIR_AHEAD].dist;
			current = current->edge[DIR_AHEAD].dest;
		}

		if (current == start) {
			// HORRY SHIITTu, WE ARE IN A ROOP!
			return max_distance;
		}

		if (distance >= max_distance) {
			return max_distance;
		}

	}

	distance += end_offset;

	// make sure distance is >= 0, and <= max_distance:
	return MIN(MAX(0, distance), max_distance);
}

int next_sensor_node(int start_num,	int direction, int *target_distance) {
	struct track_node *start_node = &track[start_num];

	AssertF(track != 0 && start_node != 0 && target_distance != 0, "BAD INPUT TO NEXT_SENSOR_NODE", 0);

	struct track_node *current = start_node;
	*target_distance = 0;
	while (1) {
		if (current->type == NODE_EXIT) {
			break;
		} else if (current->type == NODE_BRANCH) {

			int switch_direction = switch_table[current->num];
			if (direction != -1) {
				switch_direction = direction;
			}

			AssertF(switch_direction != -1, "SwitchGetDirection returned -1\r");

			int node_direction = (switch_direction == SWITCH_STRAIGHT ? DIR_STRAIGHT : DIR_CURVED);

			*target_distance += current->edge[node_direction].dist;
			current = current->edge[node_direction].dest;
		} else {
			*target_distance += current->edge[DIR_AHEAD].dist;
			current = current->edge[DIR_AHEAD].dest;
		}

		// if the sensor is broken, we must skip this one!
		if (current->type == NODE_SENSOR) {
			return current->num;
		}
		if (current == start_node) {
			// we are in a loop!
			Assert("Next Sensor Node is in a loop!");
		}
	}

	return -1;
}

// Entry point of track-server
void track_server_entry() {

	RegisterAs(TRACK_SERVER);

	// Initialize the track
	track_init();

	// Initialize the reservation list
	reservation_init(train_reservations);

	// So we can update our own cached copy of the switch table
	int switch_courier_tid = CreateSwitchCourier(PRIORITY_HIGHEST - 1);

	int sender_tid;

	// Message buffer large enough to hold the largest message request
	char message_buffer[32];
	while (1) {
		Receive(&sender_tid, message_buffer, sizeof(message_buffer));

		if (sender_tid == switch_courier_tid) {
			// This is an update from the switch server, record the switch that changed
			// so that our cached table of switches is in sync
			int reply = 0;
			Reply(sender_tid, (char*)&reply, sizeof(reply));
			struct switch_report* report = (struct switch_report*)message_buffer;
			switch_table[(int)report->sw] = report->direction;
		} else {
			// API requests to the track_server

			struct track_reply_message reply = {-1, -1};
			int op = *(int*)message_buffer;

			switch(op) {

				// Handle track reservation requests
				case TRACK_OP_RESERVATION: {
					int reply = -1;
					struct reservation_request_message* reservation_msg = (struct reservation_request_message*)message_buffer;

					// Make the location be a sensor and positive offset that fits in the edge
					if (normalize_location(track, switch_table, &reservation_msg->position) != -1) {
						// Check if we can reserve this space for the train
						int status = reservation_verify(reservation_msg->train, &(reservation_msg->position), reservation_msg->length, reservation_msg->branch_safety);
						reply = status;

						if (status == 0) {
							// We succeeded. Reserve the track

							// Find the train's reservation_node
							struct reservation_node* train_r = get_reservation(train_reservations, reservation_msg->train);
							if (train_r != 0) {
								clear_train_reservations(train_r);

								// Add new reservation
								reservation_insert(train_r,
												&(reservation_msg->position),
												reservation_msg->length,
												reservation_msg->branch_safety);
							}
						} else if (status == -2) {
							// Reached max reservation
						}
					} else {
						Printf("Could not normalize\r");
					}

					Reply(sender_tid, (char*)&reply, sizeof(reply));
				} break;

				case TRACK_OP_RESERVATION_STRING: {
					struct reservation_request_message* reservation_msg = (struct reservation_request_message*)message_buffer;
					struct reservation_node* train_r = get_reservation(train_reservations, reservation_msg->train);

					char buf[300];
					*buf = 0; // null termination character
					int len = 0;
					if (train_r != 0) {
						len = reservation_print(train_r, buf);
					}

					Reply(sender_tid, (char*)buf, len); // +1 for null termination char
				} break;

				case TRACK_OP_NODE_IS_RESERVED: {

					struct reservation_request_message* reservation_msg = (struct reservation_request_message*)message_buffer;
					int reserved = node_is_reserved(reservation_msg->train, reservation_msg->position.node);

					Reply(sender_tid, (char*)&reserved, sizeof(reserved));
				} break;

				// Handle requests for calculating the distance between 2 nodes
				case TRACK_OP_TRACK_DISTANCE: {
					struct track_request_message* track_msg = (struct track_request_message*)message_buffer;
					if (track_msg->node1 < TRACK_MAX && track_msg->node2 < TRACK_MAX) {
						reply.distance = dist_between_nodes(track_msg->node1, track_msg->node2);
					}

					Reply(sender_tid, (char*)&reply, sizeof(reply));
				} break;

				case TRACK_OP_TRACK_DISTANCE_UNTIL_GOAL: {
					struct track_request_message* track_msg = (struct track_request_message*)message_buffer;
					int distance = 0;
					if (track_msg->node1 < TRACK_MAX && track_msg->node2 < TRACK_MAX) {
						distance = dist_until_goal(track_msg->node1, track_msg->offset1, track_msg->node2, track_msg->offset2, track_msg->distance);
					}

					Reply(sender_tid, (char*)&distance, sizeof(distance));
				} break;

				// Handle requests for getting the next sensor on the track
				case TRACK_OP_TRACK_NEXT_SENSOR: {
					struct track_request_message* track_msg = (struct track_request_message*)message_buffer;
					if (track_msg->node1 >= A1 && track_msg->node1 <= E16) {
						int distance;
						int next_sensor = next_sensor_node(track_msg->node1, track_msg->node2, &distance);

						reply.node = next_sensor;
						reply.distance = distance;
					}

					Reply(sender_tid, (char*)&reply, sizeof(reply));
				} break;

				case TRACK_OP_NORMALIZE: {
					struct track_request_message *msg = (struct track_request_message*)message_buffer;

					struct location loc;
					loc.node = msg->node1;
					loc.offset = msg->offset1;
					if (normalize_location(track, switch_table, &loc) == -1) {
						Reply(sender_tid, (char*)&loc, 0); // reply size 0 means error!
					}

					Reply(sender_tid, (char*)&loc, sizeof(loc));
				} break;

				// Handle requests for getting the reverse node of a given node
				case TRACK_OP_TRACK_REVERSE_NODE: {
					struct track_request_message* track_msg = (struct track_request_message*)message_buffer;
					if (track_msg->node1 < TRACK_MAX) {
						reply.node = track[track_msg->node1].reverse->num;
					}

					Reply(sender_tid, (char*)&reply, sizeof(reply));
				} break;

				case TRACK_OP_ROUTE: {
					struct track_request_message* track_msg = (struct track_request_message*)message_buffer;
					int train = track_msg->train;
					int start_node_num = track_msg->node1;
					int end_node_num = track_msg->node2;

					short path[TRACK_MAX];
					int num_nodes = 0;

					if (IS_SENSOR(start_node_num) && IS_SENSOR(end_node_num)) {
						struct track_node *start_node = &track[start_node_num];
						struct track_node *end_node = &track[end_node_num];
						mark_path(train, track, start_node, end_node);

						if (end_node->routing_info.visited == 1) {
							// we have a path!
							// store all nodes, from bottom up
							struct track_node *cur_node = end_node;
							struct location loc;
							while (cur_node->routing_info.previous != 0 && cur_node != start_node) {
								node_to_location(cur_node, &loc);
								path[num_nodes] = loc.node;
								num_nodes++;

								if (cur_node->routing_info.previous == cur_node->reverse) {
									// the next node is the reverse of this node, so we squeeze in a "REVERSE" in the path
									path[num_nodes] = -99; // magic number for REVERSE commands
									num_nodes++;
								}

								cur_node = cur_node->routing_info.previous;
							}

							// we didn't store the beginning node in our path, so we should do that now
							if (cur_node == start_node) {
								node_to_location(start_node, &loc);
								path[num_nodes] = loc.node;
								num_nodes++;
							}

						} else {
							Printf("Could not find route! num_nodes = %d\r", num_nodes);
						}

					}

					Reply(sender_tid, (char*)path, sizeof(short) * num_nodes);
				} break;

				case TRACK_OP_SWITCH_DIRECTION: {

					struct track_request_message* track_msg = (struct track_request_message*)message_buffer;

					int switch_node = track_msg->node1;
					int next_node = track_msg->node2;

					AssertF(track[switch_node].type == NODE_BRANCH, "TrackSwitchDirection not given a switch node! Given node num %d, type %d", track[switch_node].num, track[switch_node].type);

					int reserved = node_is_reserved(track_msg->train, switch_node);

					if (reserved) {
//						Printf("Attempting to switch %s if reserved .. \r", track[switch_node].name);
						int direction = -1;


						if (track[switch_node].edge[DIR_CURVED].dest == &(track[next_node])) {
							direction = SWITCH_CURVED;
						} else {
							direction = SWITCH_STRAIGHT;
						}

						SwitchSetDirection(track[switch_node].num, direction, WhoIs(SWITCH_SERVER));
						switch_table[track[switch_node].num] = direction;
					}

					Reply(sender_tid, (char*)&reserved, sizeof(reserved));
				} break;

				default:
					AssertF(0, "Invalid message %d to the track server from %d", op, sender_tid);
					Reply(sender_tid, (char*)&reply, sizeof(reply));
					break;
			}
		}
	}
	Assert("Track server is quitting");
	Exit();
}

// Reserves a section of track if possible returning 0, otherwise returns -1
int TrackReserve(int train, struct location* loc, int length, int branch_safety, int tid) {
	int reply;
	struct reservation_request_message msg;
	msg.operation = TRACK_OP_RESERVATION;
	msg.train = train;
	msg.position = *loc;
	msg.length = length;
	msg.branch_safety = branch_safety;
	Send(tid, (char*)&msg, sizeof(msg), (char*)&reply, sizeof(reply));
	return reply;
}

int TrackReservationString(int train, char *buf, int len, int tid) {
	struct reservation_request_message msg;
	msg.operation = TRACK_OP_RESERVATION_STRING;
	msg.train = train;

	int reply = 0;
	reply = Send(tid, (char*)&msg, sizeof(msg), (char*)buf, len);
	return reply;
}

int TrackNodeIsReserved(int train, int node, int tid) {
	int reply;

	struct reservation_request_message msg;
	msg.operation = TRACK_OP_NODE_IS_RESERVED;
	msg.train = train;
	msg.position.node = node;
	msg.position.offset = 0;

	Send(tid, (char*)&msg, sizeof(msg), (char*)&reply, sizeof(reply));
	return reply;
}

// Returns next sensor, modifies distance with distance traveled to get to next sensor
int TrackNextSensor(int sensor, int *distance, int tid) {
	return TrackNextSensorDirected(sensor, -1, distance, tid);
}

// gets normalized version of node+offset
int TrackNormalize(struct location *loc, struct location *target_loc, int tid) {

	struct track_request_message msg;
	msg.operation = TRACK_OP_NORMALIZE;
	msg.node1 = loc->node;
	msg.offset1 = loc->offset;

	return Send(tid, (char*)&msg, sizeof(msg), (char*)target_loc, sizeof(struct location));
}

// Gets the next sensor given the direction to take if a branch should appear
int TrackNextSensorDirected(int sensor, int direction, int *distance, int tid) {
	struct track_request_message msg;
	msg.operation = TRACK_OP_TRACK_NEXT_SENSOR;
	msg.node1 = sensor;
	msg.node2 = direction;

	struct track_reply_message reply;
	Send(tid, (char*)&msg, sizeof(msg), (char*)&reply, sizeof(reply));

	if (distance != 0) {
		*distance = reply.distance;
	}
	return reply.node;
}

// Returns the distance between two nodes
int TrackDistanceBetweenNodes(int node1, int node2, int tid) {
	struct track_request_message msg;
	msg.operation = TRACK_OP_TRACK_DISTANCE;
	msg.node1 = node1;
	msg.node2 = node2;

	struct track_reply_message reply;
	Send(tid, (char*)&msg, sizeof(msg), (char*)&reply, sizeof(reply));

	return reply.distance;
}

// Returns the distance between node1,offset to travel to node2,offset, up to maximum 'distance'
int TrackDistanceUntilGoal(struct location *start, struct location *end, int distance, int tid) {
	struct track_request_message msg;
	msg.operation = TRACK_OP_TRACK_DISTANCE_UNTIL_GOAL;
	msg.node1 = start->node;
	msg.offset1 = start->offset;

	msg.node2 = end->node;
	msg.offset2 = end->offset;

	msg.distance = distance;

	int reply;
	Send(tid, (char*)&msg, sizeof(msg), (char*)&reply, sizeof(reply));

	return reply;
}

// Returns the reverse node of this node
int TrackReverseNode(int node, int tid) {
	struct track_request_message msg;
	msg.operation = TRACK_OP_TRACK_REVERSE_NODE;
	msg.node1 = node;

	struct track_reply_message reply;
	Send(tid, (char*)&msg, sizeof(msg), (char*)&reply, sizeof(reply));

	return reply.node;
}

int TrackRoute(int train, int start_node, int end_node, short *path, int path_size, int tid) {
	struct track_request_message msg;
	msg.operation = TRACK_OP_ROUTE;
	msg.train = train;
	msg.node1 = start_node;
	msg.node2 = end_node;

	int retval = Send(tid, (char*)&msg, sizeof(msg), (char*)path, path_size);
	return (retval / sizeof(short));
}

int TrackSwitchDirection(int train, int switch_node, int next_node, int tid) {
	int reserved;

	struct track_request_message msg;
	msg.operation = TRACK_OP_SWITCH_DIRECTION;
	msg.train = train;
	msg.node1 = switch_node;
	msg.node2 = next_node;

	AssertF(Send(tid, (char*)&msg, sizeof(msg), (char*)&reserved, sizeof(reserved)) >= 0, "Send() failed in TrainSwitchDirection");
	return reserved;
}
