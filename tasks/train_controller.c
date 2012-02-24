#include <train_controller.h>
#include <syscall.h>
#include <name_server.h>

#include <track_data.h>
#include <track_node.h>
#include <train_profile.h>

#include <track_server.h>
#include <train_command.h>
#include <train.h>
#include <clock.h>
#include <sensor.h>
#include <switch.h>
#include <term.h>

#include <string.h>

#define CALIBRATING 0

#define MAX_NUM_TRAINS 4
#define TRAIN_MAP_SIZE 42
#define IS_TRAIN(x) ((x) >= 0 && (x) <= TRAIN_MAP_SIZE)

#define TRAIN_CONTROLLER_SPEED		0
#define TRAIN_CONTROLLER_STOP		1
#define TRAIN_CONTROLLER_POSITION	2
#define TRAIN_CONTROLLER_REVERSE 	3
#define TRAIN_CONTROLLER_REVERSE_RESET		4
#define TRAIN_CONTROLLER_ROUTE 5
#define TRAIN_CONTROLLER_ROUTE_STOP 8

#define SENSOR_FLIP_FLOP_INTERVAL	20
#define PING_DELAY	(20 MSEC)
#define TRAIN_REROUTE_TIME (2 * TICKS_PER_SEC)

#define SPEED_INDEX_TO_SPEED(x) ((x) > 14 ? (x) - 14 : (x))

static int term_tid, track_tid, clock_tid;
static int actual_segment_ticks;
static int estimated_segment_ticks;

static struct train_profile *train_map[TRAIN_MAP_SIZE];
static struct train_profile train41;
static struct train_profile train35;
static struct train_profile train36;

static struct track_node track[TRACK_MAX];

void printSensorTicks() {
	/*int i, j;
	int speed = 1;
	for (speed = 8; speed <= 14; speed++) {
		for (i = A1; i <= E16; i++) {
			for (j = A1; j <= E16; j++) {
				if (train36.track_segment_ticks[speed][i][j] > 0)
					bwprintf(COM2, "t[%d][%d][%d] = %d; ", speed, i, j, train36.track_segment_ticks[speed][i][j]);
			}
		}
	}*/
}

// Calculates the velocity of the train since the last snapshot
static inline int calculate_velocity(struct train_profile* train, int now) {
	int vel = train->snapshot.velocity;
	int dt = now - train->snapshot.time;

	if (train->snapshot.type == TYPE_ACCELERATING) {
		int dt1 = train->snapshot.time - train->snapshot.last_action_time;
		int dt2 = now - train->snapshot.last_action_time;
		vel += train->accelerating_velocity_func(dt2) - train->accelerating_velocity_func(dt1);
		vel = MIN(vel, train->snapshot.target_velocity);
	} else if (train->snapshot.type == TYPE_DECELERATING) {
		vel += (train->snapshot.deceleration * dt) / TICKS_PER_SEC;
		vel = MAX(vel, train->snapshot.target_velocity);
	}

	return vel;
}

// Update the snapshot at this time
static int update_position(struct train_profile* train, int now) {
	int dt = now - train->snapshot.time;

	// Calculate the train's position since the last snapshot
	int position = train->snapshot.position;
	if (train->snapshot.type == TYPE_ACCELERATING) {
		int dt1 = train->snapshot.time - train->snapshot.last_action_time;
		int dt2 = now - train->snapshot.last_action_time;
		position += train->accelerating_position_func(dt2) - train->accelerating_position_func(dt1);
	} else if (train->snapshot.type == TYPE_DECELERATING) {
		// Constant deceleration
		position += (train->snapshot.velocity * dt) / TICKS_PER_SEC;
		position += (train->snapshot.deceleration * dt * dt) / (2 * TICKS_PER_SEC * TICKS_PER_SEC);
	} else {
		// Constant velocity
		position += (train->snapshot.velocity * dt) / TICKS_PER_SEC;
	}

	if (train->latest_sensor_node != -1) {
		int dist;
		if (TrackNextSensor(train->latest_sensor_node, &dist, track_tid) >= 0) {
			position = MIN(position, dist);
		}
	}

	// Calculate the velocity of the train since the last snapshot
	int velocity = calculate_velocity(train, now);
	if (velocity == train->snapshot.target_velocity && train->snapshot.type != TYPE_CONSTANT) {
		// The train has reached it's target velocity, so
		// update the snapshot
		//Printf("Changing type to constant because velocity (%d) matches target velocity (%d)\r", velocity, train->snapshot.target_velocity);
		train->snapshot.type = TYPE_CONSTANT;
		train->snapshot.velocity = velocity;
		train->snapshot.position = position - train->snapshot.position;
		if (train->direction == DIRECTION_STRAIGHT) {
			train->snapshot.position += train->truck_front;
		} else {
			train->snapshot.position += train->length - train->truck_back;
		}
		train->snapshot.deceleration = 0;
		train->snapshot.time = now;
		train->snapshot.last_action_time = now;
		train->snapshot.last_speed_index = train->current_speed;
	}

	return position;
}

// Calculates the deceleration right now if the train were to stop
static int calculate_deceleration(struct train_profile* train, int time) {
//	int velocity = calculate_velocity(train, time);
	int velocity = train->avg_velocities[train->speed_index];

	AssertF(train->stopping_distance[train->speed_index] > 0, "Stopping distance for train %d is zero at speed %d", train->num, train->speed_index);

	int deceleration = (velocity * velocity) / train->stopping_distance[train->speed_index];
	return -deceleration / 2;
}

static int calculate_stopping_distance(struct train_profile* train, int time) {
	int velocity = calculate_velocity(train, time);
	return train->stopping_distance_func(velocity);
}

static void train_set_speed(int train, int speed) {
	if (TrainSpeed(speed, train_map[train]->tid) >= 0) {
		if (speed != train_map[train]->current_speed) {
//			Printf("SET TRAIN %d SPEED %d\r", train, speed);
			int time = Time(clock_tid);

			// save our current speed
			train_map[train]->previous_speed_index =
					train_map[train]->speed_index;

			// Calculate the velocity and position the train should be at now
			int velocity = calculate_velocity(train_map[train], time);
			int position = update_position(train_map[train], time);

			// Update the snapshot to now and set the velocity to what
			// was calculated above
			train_map[train]->snapshot.time = time;
			train_map[train]->snapshot.last_action_time = time;
			train_map[train]->snapshot.velocity = velocity;
			train_map[train]->snapshot.position = position;
			if (speed != 0) {
				train_map[train]->paused = 0;
			}

			if (speed == 0) {
				train_map[train]->snapshot.type = TYPE_DECELERATING;
				train_map[train]->snapshot.deceleration = calculate_deceleration(train_map[train], time);
				train_map[train]->speed_index = 0;
			} else if (speed < train_map[train]->current_speed) {
				// We are decelerating

				// Set the new snapshot to have an acceleration as if we were
				// decelerating from the past speed to 0
				train_map[train]->snapshot.type = TYPE_DECELERATING;
				train_map[train]->snapshot.deceleration = calculate_deceleration(train_map[train], time);
				train_map[train]->speed_index = 14 + speed;
			} else {
				// we are accelerating

				// Set the new snapshot to have an acceleration as if we were
				// accelerating from 0 to the specified speed
				train_map[train]->snapshot.type = TYPE_ACCELERATING;
				train_map[train]->speed_index = speed;
			}

			train_map[train]->current_speed = speed;

			// Update the target velocity
			int distance;
			int next_node = TrackNextSensor(train_map[train]->latest_sensor_node, &distance, track_tid);
			train_map[train]->snapshot.target_velocity = (distance * TICKS_PER_SEC)
					/ train_map[train]->track_segment_ticks[train_map[train]->speed_index][train_map[train]->latest_sensor_node][next_node];
		}
	} else {
		TermPrint("Uhoh, something bad happened while setting speed @ train controller\r", term_tid);
	}
}

static void train_reverse(int train) {
	AssertF(train > 0 && train < TRAIN_MAP_SIZE, "Invalid train  number %d", train);
	AssertF(train_map[train] != 0, "reverse: Invalid train %d [addr=%x]", train, train_map[train]);

	int old_speed = train_map[train]->current_speed;
	train_set_speed(train, 0);
	TrainReverse(old_speed, train_map[train]->tid);
}

static int train_plan_route(int train, int end_sensor) {

	if (IS_TRAIN(train) && train_map[train] != 0) {
		int start_sensor = train_map[train]->latest_sensor_node;

		AssertF(IS_SENSOR(start_sensor), "Invalid train's next sensor: %s");
		AssertF(IS_SENSOR(end_sensor), "Invalid train's next sensor: %s");
		TermPrintf(term_tid, "Train %d: route from %s -> %s:\r", train, track[start_sensor].name,
				track[end_sensor].name);

		int num_nodes = TrackRoute(train, start_sensor, end_sensor,
				train_map[train]->route.path,
				sizeof(train_map[train]->route.path),
				track_tid);

		train_map[train]->route.rev_cursor = -1;
		train_map[train]->route.started = 0;
		train_map[train]->route.errors = 0;

		if (num_nodes != 0) {
			train_map[train]->route.cursor = num_nodes - 1;
		} else {
			train_map[train]->route.cursor = -1;
			return -1;
		}

		// train->route.path now contains the route
		// num_nodes is the number of nodes in the TrackRoute

		train_set_speed(train, train_map[train]->default_speed);
		if (train_map[train]->route.path[train_map[train]->route.cursor-1] == -99) {
			// first thing we do is we reverse, so let's do that now
			// set the reverse node in this position as -1 so it's not looked at again at the bottem

			train_map[train]->route.path[train_map[train]->route.cursor-1] = -1;
			train_map[train]->route.cursor -= 2;
			train_reverse(train);
		}

		// rev_cursor contains the first REVERSE, if it exists. -1 otherwise
		int i;
		for (i = num_nodes - 1; i >= 0; --i) {
			int node = train_map[train]->route.path[i];
			if (node == -99) {
				if (train_map[train]->route.rev_cursor == -1) {
					train_map[train]->route.rev_cursor = i;
				}

				TermPrintf(term_tid, "-> REV ");
			} else {
				TermPrintf(term_tid, "-> %s ", track[node].name);
			}
		}

	} else {
		TermPrintf(term_tid, "Route finding error: Invalid Train %d\r", train);
		return -1;
	}

	TermPrint("\r", term_tid);
	return 0;
}

static void update_route_path(struct train_profile *train, int triggered_node) {
	// if train is following path..
	if ((train->route.cursor != -1) && (triggered_node != -1)) {

		if (train->route.path[train->route.cursor] != triggered_node) {
			// we can move the cursor forward until we are at latest_sensor_node, given it's within 2 sensors in our path

			int cursor_initially = train->route.cursor;

			int sensors_encountered = 0;
			while (sensors_encountered < 2 && train->route.path[train->route.cursor] != triggered_node) {
				int i = train->route.cursor - 1;

				// skip until i points at a sensor:
				while (i >= 0 && !IS_SENSOR(train->route.path[i])) {
					i--;
				}

				// could we find it?
				if (i < 0) {
					break;
				} else {
					//Printf("Next sensor is our planned route is %s\r", track[train->route.path[i]]);
					sensors_encountered++;
					train->route.cursor = i;
				}
			}

			// going forward hasn't gotten us to our current sensor. restore!
			if (train->route.path[train->route.cursor] != triggered_node) {
				if (train->route.started == 0) {
					// The train hasn't started/reached the first sensor in the path yet, so defer any crazy decisions
					train->route.cursor = cursor_initially;
				}
				else {

					train->route.errors++;

					if (train->route.errors >= 3) {
						// The train has started the path, but we don't know where it is, so recalculate a new path
						Printf("Don't know where the train is (cursor @ %s, last_sensor is %s).. creating new path\r",
								track[train->route.path[train->route.cursor]].name,
								track[triggered_node].name);

						train_plan_route(train->num, train->route.path[0]);

					}
				}

			} else {
				train->route.errors = 0;
				train->route.started = 1;
//				Printf("cursor is now pointing at %s\r", track[train->route.path[train->route.cursor]].name);
			}
		} else {
			// The latest sensor node is the next node on the path. So
			train->route.started = 1;
		}

	}
}

// Updates snapshots of trains and calibrates existing track data
static void train_controller_sensor_update(struct train_profile* train, int triggered_node, int time) {
	int time_diff = time - train->latest_sensor_timestamp;

	int distance;
	int next_node = TrackNextSensor(triggered_node, &distance, track_tid);

	if (train->latest_sensor_node != -1) {
		// Update the snapshot

		if (train->snapshot.type == TYPE_CONSTANT) {
			// We are not accelerating, so we can use the velocities predicted
			// for the next track segment
			int dt = train->track_segment_ticks[train->speed_index][triggered_node][next_node];
			train->snapshot.velocity = (distance * 100) / dt;
			train->snapshot.last_action_time = time;
		} else {
			// We are accelerating, so let's calculate our velocity from previous
			// acceleration and use that
			train->snapshot.velocity = calculate_velocity(train, time);
		}

		if (train->snapshot.type == TYPE_CONSTANT) {
			/** DYNAMIC CALIBRATION: **/
			short *ticks = &(train->track_segment_ticks[train->speed_index][train->latest_sensor_node][triggered_node]);
			short *ticks_freq =	&(train->track_segment_ticks_freq[train->speed_index][train->latest_sensor_node][triggered_node]);

			actual_segment_ticks = time_diff;
			estimated_segment_ticks = *ticks;

			// *ticks is estimated, time_diff is measured
			int diff = ABS(*ticks - time_diff);
			if (diff >= 40 && CALIBRATING) {
				// our estimated value is larger than it should be. let's replace it altogether with this new actual
				Printf("Diff too large (%d) -- replacing!\r", *ticks - time_diff);
				*ticks = time_diff;
			} else if (diff < 40) {
				// wait the new actual with our estimate
				int new_avg = (((*ticks) * (*ticks_freq)) + time_diff) / (*ticks_freq + 1);
				*ticks_freq += 1;

				*ticks = new_avg;
			}
		}
	}

	// Since we hit a sensor, our snapshot is now position 0 from sensor
	if (train->direction == DIRECTION_STRAIGHT) {
		train->snapshot.position = train->truck_front;
	} else {
		train->snapshot.position = train->length - train->truck_back;
	}
	train->snapshot.target_velocity = (distance * TICKS_PER_SEC) / train->track_segment_ticks[train->speed_index][triggered_node][next_node];
	train->snapshot.time = time;

	// Update the route path to reflect the sensor we just hit
	update_route_path(train, triggered_node);

	train->latest_sensor_timestamp = time;
	train->latest_sensor_node = triggered_node;
}

// Finds the train that most likely tripped this sensor
// Train within range of sensor gets the sensor
static int attribute_sensor(int triggered_sensor, int time) {
	int reverse_sensor = TrackReverseNode(triggered_sensor, track_tid);

	unsigned int closest_proximity = -1;
	int closest_train = -1;

	int train;
	for (train = 0; train < TRAIN_MAP_SIZE; train++) {
		if (train_map[train] != 0) {
			// This is a valid train

			if (train_map[train]->latest_sensor_node == -1) {
				// This train has not been discovered, so assuming that trains start being discovered
				// in increasing order of train number, this should be the train
				Printf("Discovering train %d\r", train);
				train_set_speed(train, 0);
				return train;
			} else if (train_map[train]->latest_sensor_node	== reverse_sensor && time - train_map[train]->latest_sensor_timestamp <= SENSOR_FLIP_FLOP_INTERVAL) {
				// This must be a flip-flop
				return -2;
			} else if (train_map[train]->latest_sensor_node != triggered_sensor) {
				// This might be a legitimate sensor hit for this train

				// Calculate our position from our last sensor
				int position = update_position(train_map[train], time);

				int distance;
				int next_node = TrackNextSensor(train_map[train]->latest_sensor_node, &distance, track_tid);
				distance = ABS(distance - position);
				if (next_node == triggered_sensor) {
					// The triggered sensor is on the path, so let's assume this can belong to
					// this train. Enter it as a candidate
					if (distance < closest_proximity) {
						closest_proximity = distance;
						closest_train = train;
					}
				} else {
//					Printf("Train %d expected %s, checking next in path...", train, get_track()[next_node].name);
					// Looks like we missed the next sensor
					int distance2;
					next_node = TrackNextSensor(next_node, &distance2, track_tid);

					if (next_node == triggered_sensor) {
						// This is possibly also ours, let's consider it
						distance += distance2;
//						Printf("candidate with distance %d\r", distance);
						if (distance < closest_proximity) {
							closest_proximity = distance;
							closest_train = train;
						}
					} else {
//						Printf("expected %s\r", train, get_track()[next_node].name);
					}
				}
			}
		}
	}

	return closest_train;
}

static int recover_train(int triggered_sensor, int time) {
	int train;
	for (train = 0; train < TRAIN_MAP_SIZE; train++) {
		if (train_map[train] != 0) {
			// This is a valid train
			int distance;

			int next_sensor = TrackNextSensorDirected(train_map[train]->latest_sensor_node, SWITCH_STRAIGHT, &distance, track_tid);
			if (next_sensor == triggered_sensor) {
				return train;
			}

			next_sensor = TrackNextSensorDirected(train_map[train]->latest_sensor_node, SWITCH_CURVED, &distance, track_tid);
			if (next_sensor == triggered_sensor) {
				return train;
			}
		}
	}
	return -1;
}

static void train_controller_init() {
	// Declare trains here
	memset(train_map, 0, sizeof(train_map));
	train_map[41] = &train41;
	train_map[35] = &train35;
//	train_map[36] = &train36;

	train41_init(&train41, track_tid);
	train35_init(&train35, track_tid);
	train36_init(&train36, track_tid);

	train41.tid = CreateTrainTask(PRIORITY_HIGHEST - 2, 41);
	train35.tid = CreateTrainTask(PRIORITY_HIGHEST - 2, 35);
	train36.tid = CreateTrainTask(PRIORITY_HIGHEST - 2, 36);
}

void delayed_pinger_entry() {

	/* create this task if you want it to ping it's parent every tick */

	int ptid = MyParentTid();
	int clock_tid = WhoIs(CLOCK_SERVER);

	int msg = 0;
	int reply = 0;
	while (1) {
		Delay(PING_DELAY, clock_tid);
		Send(ptid, (char*) &msg, sizeof(msg), (char*) &reply, sizeof(reply));
	}
}

static int train_route_stop(int train, int status) {

	AssertF(train >= 0 && train <= TRAIN_MAP_SIZE && train_map[train] != 0, "Invalid train");

	train_map[train]->route.cursor = -1;
	train_map[train]->route.rev_cursor = -1;
	train_map[train]->route.started = 0;

	if (train_map[train]->route.reply_tid != -1) {
		Reply(train_map[train]->route.reply_tid, (char*) &status, sizeof(status));
		train_map[train]->route.reply_tid = -1;
	}

	return 0;
}

void train_controller_entry() {
	RegisterAs(TRAIN_CONTROLLER);

	init_tracka(track);
	int sensor_courier_tid = CreateSensorCourier(PRIORITY_HIGHEST - 1);
	clock_tid = WhoIs(CLOCK_SERVER);
	term_tid = WhoIs(TERM_SERVER);
	track_tid = WhoIs(TRACK_SERVER);

	train_controller_init();

	int sender_tid;
	char msg_buffer[100];

	estimated_segment_ticks = 0;
	actual_segment_ticks = 0;

	int ping_tid = Create(PRIORITY_NORMAL, &delayed_pinger_entry);

	/*** PROCESS TRAINS NOW ***/
	while (1) {
		int reply = 0;
		Receive(&sender_tid, msg_buffer, sizeof(msg_buffer));

		if (sender_tid == ping_tid) {
			struct track_node * track = get_track();
			int time_now = Time(clock_tid);

			int i;
			for (i = 0; i < TRAIN_MAP_SIZE; ++i) {
				struct train_profile *train = train_map[i];
				if (train == 0) {
					continue;
				}

				/*
				 * train is moving
				 *  - when we udpate a reservation, and there is a switch in our reservation path,
				 *    we turn that switch according to our path, in case we have a path
				 *
				 *  - whenever we are close enough to our destinaton s.t. we should stop, we should
				 *  - how do we know we are close enough?
				 *  	-
				 */

				// update_route_path(train);
				struct location loc;
				loc.node = train->latest_sensor_node;
				loc.offset = update_position(train, time_now);

				int stop_dist = 0;
				int reserve_status = -1;
				// update reservation:
				if (train->latest_sensor_node != -1) {
					stop_dist = calculate_stopping_distance(train, time_now);

					// the position we need to back track train->length (even less actually, but let's keep it safe)
					// so we can reserve enough for the back of the train!
//					if (loc.offset - train->length < 0) {
//						struct track_node *track = get_track();
//						struct track_node *cur_node =  &track[loc.node];
//
//						// if we didn't back up into a branch, we can go ahead and subtract!
//						// otherwise, fuckit.
//						if (cur_node->reverse->type != NODE_BRANCH) {
//							cur_node->reverse
//						}
//					} else {
//						loc.offset -= train->length;
//					}

					struct location reserve_loc = {loc.node, 0};
					reserve_status = TrackReserve(train->num, &reserve_loc,
													loc.offset + stop_dist + 2*train->length, // train length is just some extra precaution
													train->length, // branch safety (always makes sure that this amount is reserved *after* a branch node)
													track_tid);

					if (reserve_status == -3) {
						reserve_status = TrackReserve(train->num, &reserve_loc,
								loc.offset + stop_dist - 1, // train length is just some extra precaution
								train->length, // branch safety (always makes sure that this amount is reserved *after* a branch node)
								track_tid);
					}

					// if the train is paused, start it up again!
					if (reserve_status >= 0 && train->paused > 0 && (time_now - train->paused) > (2 SEC) && train->speed_index == 0) {
						train->paused = 0;
						train_set_speed(train->num, train->previous_speed_index);
					} if (reserve_status == -2 && train->speed_index > 0) {
						// The reservation is too big!
						Printf("The reservation is too big! Train %d must be stuck!\r", train->num);
					}

					// if train is moving (and, of course, we know it's position) ..
					if (calculate_velocity(train, time_now) > 0) {
						// if we are on a route to a destination..
						if (train->route.cursor != -1 && train->route.started == 1) {
							// are we close enough to stop?
							struct location start_loc, end_loc;
							start_loc.node = train->latest_sensor_node;
							start_loc.offset = update_position(train, time_now);
							end_loc.node = train->route.path[0];
							end_loc.offset = 0;

							int delta_d = TrackDistanceUntilGoal(&start_loc, &end_loc, stop_dist, track_tid);
							if (delta_d < stop_dist) {
								train_set_speed(train->num, 0);


								int velocity = calculate_velocity(train, time_now);
								int deceleration = calculate_deceleration(train, time_now);

								TermPrintf(term_tid, "ROUTING FINISHED: train %d. Position=%s + %d, dist_to_sensor=%d, stopping_dist=%d\r, eta=%d SEC",
										train->num, track[start_loc.node].name, start_loc.offset, delta_d, stop_dist, velocity/-deceleration);

								// reply with ETA (in ticks) to stop so that things like cab_service can wait until train stops
								train_route_stop(train->num, (velocity * TICKS_PER_SEC) / -deceleration);
							}

							// are we close enough to reverse if we need to?
							// find the next reverse node:
							if (train->route.rev_cursor != -1) {
								end_loc.node = train->route.path[train->route.rev_cursor + 1];
								end_loc.offset = train->length + (train->length/3);


//								/** SKIP TURN OUTS! **/
//								struct location train_begin;
//								train_begin.node = end_loc.node;
//								train_begin.offset = end_loc.offset;
//
//								struct location train_end;
//								train_end.node = end_loc.node;
//								train_end.offset = end_loc.offset - train->length;
//								AssertF(TrackNormalize(&train_end, &train_end, track_tid) > 0, "train controller: could not normalize end_loc when finding reverse stopping dist");
//
//								while (IS_TURNOUT(train_begin.node) || IS_TURNOUT(train_end.node)) {
//									train_begin.offset += train->length;
//									train_end.offset += train->length;
//
//									AssertF(TrackNormalize(&train_begin, &train_begin, track_tid) > 0, "train controller: could not normalize end_loc when finding reverse stopping dist");
//									AssertF(TrackNormalize(&train_end, &train_end, track_tid) > 0, "train controller: could not normalize end_loc when finding reverse stopping dist");
//									Printf("Uhoh train may stop at switch. Moving reverse stopping point!\r");
//								}
//
//								end_loc.node = train_begin.node;
//								end_loc.offset = train_begin.offset;

								int rev_dist = TrackDistanceUntilGoal(&start_loc, &end_loc, stop_dist, track_tid);
								if (rev_dist < stop_dist) {
									// reverse the train!
									train_reverse(train->num);

									TermPrintf(term_tid, "Time to reverse train %d -- pos=%s + %d, stop_dist=%d, cur_dist=%d end_node=%s %d\r",
											train->num, track[loc.node].name, loc.offset, stop_dist, rev_dist, track[end_loc.node].name, end_loc.offset);

									// update rev_cursor
									--train->route.rev_cursor;
									for (; train->route.rev_cursor >= 0; --train->route.rev_cursor) {
										if (train->route.path[train->route.rev_cursor] == -99) {
											break;
										}
									}
								}
							}
						}

						if (reserve_status < 0 && train->speed_index > 0) {
							// couldn't reserve. should probably stop the train now!
//							TermPrintf(term_tid, "Couldn't reserve track. Setting train %d speed to 0!\r", train->num);
							train_set_speed(train->num, 0);

							if (reserve_status != -2) {
								train->paused = time_now;
							}
						} else if (train->route.cursor != -1) {
							// if there is a switch in our path that is reserved and not switched, switch it!
							int i;
							for (i = train->route.cursor + 2; i >= 0; --i) {
								int node = train->route.path[i];

								if (node != -1 && IS_SWITCH_NODE(node)) {
									AssertF(i - 1 >= 0, "Uhoh, last node in our path is not a sensor!\r");

									int reserved = TrackSwitchDirection(train->num, node, train->route.path[i - 1], track_tid);
									if (reserved) { // that switch wasn't reserved!
//										Printf("Switched %s\r", track[train->route.path[i]].name);
										train->route.path[i] = -1; // switch has been switched. off we go!
									}
								}
							} // for
						}

					} else if (train->route.cursor >= 0
							&& train->route.started == 1
							&& train->paused > 0
							&& (time_now - train->paused) > TRAIN_REROUTE_TIME) {
						// reroute because we've paused for too long!
						train_plan_route(train->num, train->route.path[0]);
					}

				}

			}

			Reply(sender_tid, (char*) &reply, sizeof(reply));

		} else if (sender_tid == sensor_courier_tid) {
			Reply(sender_tid, (char*) &reply, sizeof(reply));
			// A sensor has been triggered. Find the train who triggered it and update the train's
			// snapshot

			struct sensor_report* report = (struct sensor_report*) msg_buffer;
			int sensor_i;
			for (sensor_i = 0; sensor_i < report->num_sensors; sensor_i++) {
				int triggered_sensor = report->sensor_nodes[sensor_i];

				// Find the train this sensor was triggered by
				int train = attribute_sensor(triggered_sensor, report->timestamp);

				if (train == -1) {
					TermPrintf(term_tid, "Expected: Checking if spurious sensor is train gone rogue...\r", term_tid);
					train = recover_train(triggered_sensor, report->timestamp);
				}

				if (train >= 0) {
//					Printf("Attributed sensor %s to train %d\r", get_track()[triggered_sensor].name, train);
					train_controller_sensor_update(train_map[train], triggered_sensor, report->timestamp);
				} else if (train == -1) {
					Printf("Spurious sensor trigger %s. ", get_track()[triggered_sensor].name);
					for (train = 0; train < TRAIN_MAP_SIZE; train++) {
						if (train_map[train] != 0) {
//							Printf("Train %d expected %s; ", train, get_track()[TrackNextSensor(train_map[train]->latest_sensor_node, 0, track_tid)].name);
						}
					}
					Printf("\r");

					train_set_speed(35, 0);
					train_set_speed(41, 0);
					train_set_speed(36, 0);
				} else if (train == -2) {
					TermPrint("Flip-flop\r", term_tid);
				}
			}

		} else {
			// General commands for update, train speed setting and train stopping
			struct train_controller_message* message = (struct train_controller_message*) msg_buffer;
			switch (message->command) {
				case TRAIN_CONTROLLER_SPEED: {
					int train = message->data[0];
					int speed = message->data[1];

					// Reply immediately to avoid deadlock
					Reply(sender_tid, (char*) &reply, sizeof(reply));

					if (train < 0 || train >= TRAIN_MAP_SIZE) {
						Printf("Out of bounds train lookup %d", train);
						break;
					} else if (train_map[train] == 0) {
						Printf("Train is invalid %d", train);
						break;
					}

					train_set_speed(train, speed);

				} break;

				case TRAIN_CONTROLLER_POSITION: {
					int train = message->data[0];
					AssertF(train > 0 && train < TRAIN_MAP_SIZE, "Invalid train  number %d", train);

					// we wanna report no position for trains that haven't been init'd
					struct train_position position;
					if (train_map[train] != 0) {
						int now = Time(clock_tid);
						position.action = train_map[train]->snapshot.type;
						position.sensor = train_map[train]->latest_sensor_node;
						position.offset = update_position(train_map[train], now);
						position.velocity = calculate_velocity(train_map[train], now);
						position.estimated_time = estimated_segment_ticks;
						position.actual_time = actual_segment_ticks;
						position.direction = train_map[train]->direction;

						if (train_map[train]->route.cursor != -1) {
							position.destination = train_map[train]->route.path[0];
						} else {
							position.destination = -1;
						}

					} else {
						position.sensor = -1;
					}

					Reply(sender_tid, (char*) &position, sizeof(position));
				} break;

				case TRAIN_CONTROLLER_REVERSE: {
					int train = message->data[0];
					// we must set the speed here instead of doing TrainSpeed()
					// otherwise, there's a dead lock between this task and the train's task
					train_reverse(train);
					Reply(sender_tid, (char*) &reply, sizeof(reply));
				} break;

				case TRAIN_CONTROLLER_REVERSE_RESET: {
					int train = message->data[0];
					AssertF(train > 0 && train < TRAIN_MAP_SIZE, "Invalid train number %d", train);
					AssertF(train_map[train] != 0, "reset: Invalid train %d [addr=%x]", train, train_map[train]);

					if (train_map[train]->direction == DIRECTION_STRAIGHT) {
						train_map[train]->direction = DIRECTION_REVERSED;
					} else {
						train_map[train]->direction = DIRECTION_STRAIGHT;
					}

					// update snapshot so that the train position is valid
					// ASSUMPTION: train is halted, so position shouldn't be changing now
					int now = Time(clock_tid);

					int distance;
					int current_position = update_position(train_map[train], now);

					int new_position;
					int next_node = TrackNextSensor(train_map[train]->latest_sensor_node, &distance, track_tid);
					if (next_node == -1) {
						// The next node is an exit, so just use the reverse of our last sensor
						train_map[train]->latest_sensor_node = TrackReverseNode(train_map[train]->latest_sensor_node, track_tid);
						new_position = 0;
					} else {
						train_map[train]->latest_sensor_node = TrackReverseNode(next_node, track_tid);
						new_position = MAX(0, (distance - current_position) - train_map[train]->length);
					}

//					Printf("latest sensor node now reversed to %s\r", track[train_map[train]->latest_sensor_node].name);
					// now we have to make calculate_position() return new_position
					// to do that, we set the distance travelled between snapshot and sensor hit be new_position
					// after snapshot, we need to have the distance measured as 0.
					// .. to do that:
					train_map[train]->snapshot.target_velocity = 0;
					train_map[train]->snapshot.velocity = 0;
					train_map[train]->snapshot.time = now;
					train_map[train]->snapshot.last_action_time = now;
					train_map[train]->snapshot.position = new_position;
					train_map[train]->latest_sensor_timestamp = 0;

					train_map[train]->stop_location.node = -1;

					Reply(sender_tid, (char*) &reply, sizeof(reply));
				} break;

				case TRAIN_CONTROLLER_ROUTE: {
					int train = message->data[0];
					int sensor_node = message->data[1];
					int blocking = message->data[2];

//					int module = message->data[1];
//					int sensor = message->data[2];

					if (train_map[train] == 0) {
						TermPrintf(term_tid, "Train %d is not registered as valid.\r", train);
					} else if (train_map[train]->latest_sensor_node == -1) {
						TermPrintf(term_tid, "Train %d has not yet been discovered!\r", train);
					} else if (train_map[train]->current_speed != 0) {
						TermPrint("That train's speed is not 0. You can only route trains that are stationary!\r", term_tid);
					} else {
						int success = train_plan_route(train, sensor_node) >= 0;
						if (success && blocking == 1) {
							train_map[train]->route.reply_tid = sender_tid;
							break;
						} else {
							train_map[train]->route.reply_tid = -1;
						}
					}

					Reply(sender_tid, (char*) &reply, sizeof(reply));
				} break;

				case TRAIN_CONTROLLER_ROUTE_STOP: {
					int train = message->data[0];

					if (train_map[train] == 0) {
						TermPrintf(term_tid, "Train %d is not registered as valid.\r", train);

					} else if (train_map[train]->latest_sensor_node == -1) {
						TermPrintf(term_tid, "Train %d has not yet been discovered!\r", train);
					} else if (train_map[train]->route.cursor == -1) {
						TermPrintf(term_tid, "Train %d is not on route for it to stop!\r", train);
					} else {
						if (train_map[train]->current_speed != 0) {
							train_set_speed(train, 0);
						}

						train_route_stop(train, -1);
					}

					Reply(sender_tid, (char*) &reply, sizeof(reply));
				} break;

				default:
					Assert("Invalid train_controller command");
					Reply(sender_tid, (char*) &reply, sizeof(reply));
					break;
			}
		}
	}
}

int TrainControllerSpeed(int train, int speed, int tid) {
	int reply;
	struct train_controller_message msg;
	msg.command = TRAIN_CONTROLLER_SPEED;
	msg.data[0] = (char) train;
	msg.data[1] = (char) speed;
	return Send(tid, (char*) &msg, sizeof(msg), (char*) &reply, sizeof(reply));
}

int TrainControllerReverse(int train, int tid) {
	int reply;
	struct train_controller_message msg;
	msg.command = TRAIN_CONTROLLER_REVERSE;
	msg.data[0] = train;
	return Send(tid, (char*) &msg, sizeof(msg), (char*) &reply, sizeof(reply));
}

int TrainControllerReverseReset(int train, int tid) {
	int reply;
	struct train_controller_message msg;
	msg.command = TRAIN_CONTROLLER_REVERSE_RESET;
	msg.data[0] = train;
	return Send(tid, (char*) &msg, sizeof(msg), (char*) &reply, sizeof(reply));
}

struct train_position TrainControllerPosition(int train, int tid) {
	struct train_position reply;
	struct train_controller_message msg;
	msg.command = TRAIN_CONTROLLER_POSITION;
	msg.data[0] = (char) train;
	Send(tid, (char*) &msg, sizeof(msg), (char*) &reply, sizeof(reply));
	return reply;
}

int TrainControllerRoute(int train, int sensor_module, int blocking, int tid) {
	int reply;
	struct train_controller_message msg;
	msg.command = TRAIN_CONTROLLER_ROUTE;
	msg.data[0] = (char) train;
	msg.data[1] = (char) sensor_module;
	msg.data[2] = (char) blocking;
	Send(tid, (char*) &msg, sizeof(msg), (char*) &reply, sizeof(reply));
	return reply;
}

int TrainControllerRouteStop(int train, int tid) {
	int reply;
	struct train_controller_message msg;
	msg.command = TRAIN_CONTROLLER_ROUTE_STOP;
	msg.data[0] = (char) train;
	Send(tid, (char*) &msg, sizeof(msg), (char*) &reply, sizeof(reply));
	return reply;
}
