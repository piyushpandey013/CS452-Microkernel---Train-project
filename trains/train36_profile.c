#include <train_profile.h>
#include <track_server.h>
#include <string.h>

#if TRACK_TYPE == TRACK_TYPE_A
static void init_track_a(struct train_profile* train, int track_tid);
#else
static void init_track_b(struct train_profile* train, int track_tid);
#endif

// Plug into wolfram alpha to get curve
// fit {0,0},{162,50},{197,100},{231,150},{249,200},{271,250},{293,300},{316,350},{329,400},{337,450},{349,500},{347,550},{356,600},{371,650},{380,700}

// Given a time in ticks, returns position in mm
static int accelerating_position(int time) {
	float t = (float)time;
	const float a = 0.000023359f;
	const float b = -0.00610949f;
	const float c = 0.800407f;
	const float d = -1.57049f;

	float result = a*(t*t*t) + b*(t*t) + c*t + d;
	return MAX(0, (int)result);
}

// Given a time in ticks, returns velocity in mm/s
static int accelerating_velocity(int time) {
	float t = (float)time;
	const float a = 0.000070077f;
	const float b = -0.012219f;
	const float c = 0.800407f;

	float result = a*(t*t) + b*t + c;
	return MAX(0, (int)(result * 100));
}

// fit {592, 860},{613,850},{592,810},{569, 720},{509,670},{464, 590},{411, 515},{362,470},{299,380},{244,300},{192,230},{134,150},{71,70},{12,10},{0,0}
static int stopping_distance(int velocity) {
	float v = (float)velocity;
	float a = 0.0000000119088f;
	float b = -0.0000136093f;
	float c = 0.00513592f;
	float d = 0.628109f;
	float e = 1.47853f;

	float result = a*(v*v*v*v) + b*(v*v*v) + c*(v*v) + d*(v) + e;
	return MIN(MAX(1, (int)result), 860);
}

// Carries the time (in ticks) taken to cross the track segment between two given sensors
// at the given speed setting
void train36_init(struct train_profile* train, int track_tid) {
	train_profile_init(train, 36);

	train->length = 220; 		// Length of train body
	train->truck_front = 25; 	// Distance of truck rail front from front of body
	train->truck_back = 75; 	// Distance of truck rail back from front of body

	train->default_speed = 13;

	train->accelerating_position_func = &accelerating_position;
	train->accelerating_velocity_func = &accelerating_velocity;
	train->stopping_distance_func = &stopping_distance;

	#if TRACK_TYPE == TRACK_TYPE_A
	init_track_a(train, track_tid);
#else
	init_track_b(train, track_tid);
#endif

	train_profile_fill_with_average_ticks(train, track_tid);
}

#if TRACK_TYPE == TRACK_TYPE_A
static void init_track_a(struct train_profile* train, int track_tid) {
	// increase or decrease stopping distance by these many centimeters
	// subtract to make it overshoot (hit and go past the sensor)
	// add to make it undershoot (stop prematurely)
	int fudge_factor = 0;

	/* first 14 are the accelerating stop distances */
	train->stopping_distance[0] = 1;
	train->stopping_distance[10] = MAX(670 + fudge_factor, 0);
	train->stopping_distance[11] = MAX(720 + fudge_factor, 0);
	train->stopping_distance[12] = MAX(810 + fudge_factor, 0);
	train->stopping_distance[13] = MAX(850 + fudge_factor, 0);
	train->stopping_distance[14] = MAX(860 + fudge_factor, 0);

	// Constant velocity measurements in ticks
	int velocities[] = {12, 71, 134, 192, 244, 299, 362, 411, 464, 509, 569, 592, 613, 592};
	int speed;
	int x, y;
	for (speed = 1; speed <= 14; speed++) {
		for (y = A1; y < E16; y++) {
			for (x = A1; x < E16; x++) {
				train->track_segment_ticks[speed][x][y] = velocities[speed - 1];
			}
		}
	}
}
#else
static void init_track_b(struct train_profile* train, int track_tid) {

}
#endif
