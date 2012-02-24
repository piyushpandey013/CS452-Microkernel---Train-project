
#ifndef __TRAIN_PROFILE_H__
#define __TRAIN_PROFILE_H__

#define DIRECTION_STRAIGHT 0
#define DIRECTION_REVERSED 1

#include <location.h>
#include <track_node.h>
#include <track_data.h>

#define TYPE_CONSTANT		0
#define TYPE_ACCELERATING	1
#define TYPE_DECELERATING	2

#define MAX_TRAIN_LENGTH 20

struct kinetic_snapshot {
	int type;				// Type (accelerating, decelerating, constant)
	int time; 				// time when this snapshot was taken
	int last_action_time;	// The time at which the last acceleration or deceleration was started
	int position;			// our position from the last sensor
	int velocity; 			// our velocity (mm/s) when we took the snapshot
	int deceleration;		// The rate at which the train should be decelerating, if appropriate

	int target_velocity; 	// velocity that we are heading to
	int last_speed_index; 	// our previous train speed (0-27)
};

struct train_profile {
	int num;
	int tid;

	// Train physical properties
	int length;
	int truck_front;
	int truck_back;

	// Train's current speed and direction settings
	int current_speed; 	// 0 - 14
	int speed_index; 	// 0 - 27 Indicates if speed was accelerated to or decelerated to
	int direction; 		// DIRECTION_STRAIGHT or DIRECTION_REVERSED

	int default_speed; // default speed we want this to train to run at (for routing, etc.)

	// when we set_speed(), we save our current speed to this guy before
	// switching to the new speed.
	// this comes in handy when our track-reservation fails, so we stop, but wanna start up again
	// once we can reserve enough space.
	int previous_speed_index;

	// this boolean value determines if we "paused"/stopped the train
	// because of lack of reservation, and should start up again once we have enough reservation
	int paused;

	// Last sensor triggered by the train
	int latest_sensor_node;
	int latest_sensor_timestamp;

	// A snapshot of the kinetic properties of the train
	struct kinetic_snapshot snapshot;

	// Node to stop at or around, specified by offset
	struct location stop_location;

	// Time it takes the train to cover a segment of track in ticks
	// First 14 speeds are the normal speeds
	// The next 13 speeds are "decreasing" speeds
	// +1 because we want the speed offsets to be from 1
	short track_segment_ticks[14 + 13 + 1][E16 + 1][E16 + 1];
	short track_segment_ticks_freq[14 + 13 + 1][E16 + 1][E16 + 1];

	// Acceleration function pointers
	int (*accelerating_position_func)(int);
	int (*accelerating_velocity_func)(int);
	int (*stopping_distance_func)(int);

	// stopping distance for accel/decel & each speed
	int stopping_distance[14 + 14 + 1];
	int avg_velocities[14+14+1];

	struct {
		short cursor; // index in the path that we are about-to or looking-to enter.
		short rev_cursor; // points to the position of next reverse node. -1 if there is no next
		int errors; // number of times the cursor didn't match latest_sensor node in our path while we are following it

		short path[TRACK_MAX];
		short started;

		int reply_tid; // tid that called TrainControllerRoute -- should reply to this after train was router (successfully or otherwise)
	} route;

	char reservation_str[300]; // reservation print string
};

void train_profile_init(struct train_profile* train, int num);
void train_profile_fill_with_average_ticks(struct train_profile* train, int track_tid);

void train41_init(struct train_profile* train, int track_tid);
void train35_init(struct train_profile* train, int track_tid);
void train36_init(struct train_profile* train, int track_tid);

#endif
