#include <train_profile.h>
#include <track_server.h>
#include <string.h>

#if TRACK_TYPE == TRACK_TYPE_A
static void init_track_a(struct train_profile* train, int track_tid);
#else
static void init_track_b(struct train_profile* train, int track_tid);
#endif

// Plug into wolfram alpha to get curve
// fit {0, 0},{152,50},{203,100},{227,150},{243,200},{263,250},{280,300},{291,350},{306,400},{323,450},{330,500},{335,550},{347,600},{356,650},{365,700},{380,750}

// Given a time in ticks, returns position in mm
static int accelerating_position(int time) {
	float t = (float)time;
	const float a = 0.0000129427f;
	const float b = 0.000965725f;
	const float c = -0.19902f;
	const float d = 1.17927f;

	float result = a*(t*t*t) + b*(t*t) + c*t + d;
	return MAX(0, (int)result);
}

// Given a time in ticks, returns velocity in mm/s
static int accelerating_velocity(int time) {
	float t = (float)time;
	const float a = 0.0000388281f;
	const float b = 0.00193145f;
	const float c = -0.19902f;

	float result = a*(t*t) + b*t + c;
	return MAX(0, (int)(result * 100));
}

static int stopping_distance(int velocity) {
	float v = (float)velocity;
	float a = 0.000000045217f;
	float b = -0.0000603672f;
	float c = 0.0267982f;
	float d = -2.66284f;
	float e = -2.13484f;

	float result = a*(v*v*v*v) + b*(v*v*v) + c*(v*v) + d*(v) + e;
	return MIN(MAX(1, (int)result), 870);
}

// Carries the time (in ticks) taken to cross the track segment between two given sensors
// at the given speed setting
void train41_init(struct train_profile* train, int track_tid) {
	train_profile_init(train, 41);

	train->length = 225; 		// Length of train body
	train->truck_front = 20; 	// Distance of truck rail front from front of body
	train->truck_back = 75; 	// Distance of truck rail back from front of body

	train->default_speed = 13;

	train->latest_sensor_node = D12;

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
	train->stopping_distance[10] = MAX(650 + fudge_factor, 0);
	train->stopping_distance[11] = MAX(710 + fudge_factor, 0);
	train->stopping_distance[12] = MAX(800 + fudge_factor, 0);
	train->stopping_distance[13] = MAX(860 + fudge_factor, 0);
	train->stopping_distance[14] = MAX(870 + fudge_factor, 0);

	// Constant velocity measurements in ticks
	train->track_segment_ticks[12][0][44] = 73;
	train->track_segment_ticks[12][2][42] = 66;
	train->track_segment_ticks[12][2][44] = 90;
	train->track_segment_ticks[12][3][31] = 73;
	train->track_segment_ticks[12][4][38] = 57;
	train->track_segment_ticks[12][5][25] = 102;
	train->track_segment_ticks[12][6][27] = 74;
	train->track_segment_ticks[12][7][38] = 84;
	train->track_segment_ticks[12][8][23] = 45;
	train->track_segment_ticks[12][9][38] = 115;
	train->track_segment_ticks[12][10][38] = 146;
	train->track_segment_ticks[12][12][44] = 102;
	train->track_segment_ticks[12][15][44] = 130;
	train->track_segment_ticks[12][16][61] = 62;
	train->track_segment_ticks[12][17][40] = 54;
	train->track_segment_ticks[12][18][33] = 30;
	train->track_segment_ticks[12][19][40] = 58;
	train->track_segment_ticks[12][20][50] = 60;
	train->track_segment_ticks[12][21][43] = 54;
	train->track_segment_ticks[12][28][49] = 80;
	train->track_segment_ticks[12][28][65] = 75;
	train->track_segment_ticks[12][29][63] = 30;
	train->track_segment_ticks[12][30][2] = 70;
	train->track_segment_ticks[12][31][36] = 72;
	train->track_segment_ticks[12][31][41] = 60;
	train->track_segment_ticks[12][33][49] = 78;
	train->track_segment_ticks[12][33][65] = 79;
	train->track_segment_ticks[12][35][37] = 131;
	train->track_segment_ticks[12][35][39] = 99;
	train->track_segment_ticks[12][36][46] = 48;
	train->track_segment_ticks[12][36][74] = 157;
	train->track_segment_ticks[12][38][74] = 126;
	train->track_segment_ticks[12][40][30] = 60;
	train->track_segment_ticks[12][41][16] = 55;
	train->track_segment_ticks[12][41][18] = 58;
	train->track_segment_ticks[12][42][79] = 55;
	train->track_segment_ticks[12][43][3] = 60;
	train->track_segment_ticks[12][44][70] = 138;
	train->track_segment_ticks[12][45][3] = 90;
	train->track_segment_ticks[12][46][59] = 60;
	train->track_segment_ticks[12][49][67] = 36;
	train->track_segment_ticks[12][50][68] = 42;
	train->track_segment_ticks[12][51][21] = 66;
	train->track_segment_ticks[12][52][69] = 60;
	train->track_segment_ticks[12][53][56] = 112;
	train->track_segment_ticks[12][53][73] = 99;
	train->track_segment_ticks[12][54][56] = 120;
	train->track_segment_ticks[12][54][73] = 110;
	train->track_segment_ticks[12][55][71] = 60;
	train->track_segment_ticks[12][56][75] = 54;
	train->track_segment_ticks[12][57][52] = 132;
	train->track_segment_ticks[12][57][55] = 120;
	train->track_segment_ticks[12][59][74] = 42;
	train->track_segment_ticks[12][60][17] = 65;
	train->track_segment_ticks[12][61][77] = 43;
	train->track_segment_ticks[12][63][77] = 47;
	train->track_segment_ticks[12][64][29] = 77;
	train->track_segment_ticks[12][65][78] = 33;
	train->track_segment_ticks[12][67][68] = 47;
	train->track_segment_ticks[12][68][53] = 60;
	train->track_segment_ticks[12][69][51] = 42;
	train->track_segment_ticks[12][71][45] = 138;
	train->track_segment_ticks[12][72][52] = 102;
	train->track_segment_ticks[12][73][76] = 60;
	train->track_segment_ticks[12][74][57] = 72;
	train->track_segment_ticks[12][76][60] = 42;
	train->track_segment_ticks[12][77][72] = 61;
	train->track_segment_ticks[12][79][64] = 30;
	train->track_segment_ticks[13][0][44] = 70;
	train->track_segment_ticks[13][2][42] = 61;
	train->track_segment_ticks[13][2][44] = 84;
	train->track_segment_ticks[13][3][31] = 61;
	train->track_segment_ticks[13][4][38] = 54;
	train->track_segment_ticks[13][5][25] = 98;
	train->track_segment_ticks[13][6][27] = 71;
	train->track_segment_ticks[13][7][38] = 81;
	train->track_segment_ticks[13][8][23] = 43;
	train->track_segment_ticks[13][9][38] = 110;
	train->track_segment_ticks[13][10][38] = 140;
	train->track_segment_ticks[13][12][44] = 98;
	train->track_segment_ticks[13][15][44] = 125;
	train->track_segment_ticks[13][16][61] = 58;
	train->track_segment_ticks[13][17][40] = 54;
	train->track_segment_ticks[13][18][33] = 30;
	train->track_segment_ticks[13][19][40] = 57;
	train->track_segment_ticks[13][20][50] = 59;
	train->track_segment_ticks[13][21][43] = 48;
	train->track_segment_ticks[13][28][49] = 76;
	train->track_segment_ticks[13][28][65] = 73;
	train->track_segment_ticks[13][29][63] = 32;
	train->track_segment_ticks[13][30][2] = 70;
	train->track_segment_ticks[13][31][36] = 67;
	train->track_segment_ticks[13][31][41] = 54;
	train->track_segment_ticks[13][32][19] = 30;
	train->track_segment_ticks[13][33][49] = 75;
	train->track_segment_ticks[13][33][65] = 76;
	train->track_segment_ticks[13][35][37] = 125;
	train->track_segment_ticks[13][35][39] = 95;
	train->track_segment_ticks[13][36][46] = 42;
	train->track_segment_ticks[13][36][74] = 144;
	train->track_segment_ticks[13][37][30] = 67;
	train->track_segment_ticks[13][38][74] = 121;
	train->track_segment_ticks[13][40][30] = 55;
	train->track_segment_ticks[13][41][16] = 51;
	train->track_segment_ticks[13][41][18] = 57;
	train->track_segment_ticks[13][42][20] = 48;
	train->track_segment_ticks[13][42][79] = 54;
	train->track_segment_ticks[13][43][3] = 54;
	train->track_segment_ticks[13][44][70] = 127;
	train->track_segment_ticks[13][45][3] = 84;
	train->track_segment_ticks[13][46][59] = 57;
	train->track_segment_ticks[13][47][37] = 42;
	train->track_segment_ticks[13][48][32] = 75;
	train->track_segment_ticks[13][49][67] = 34;
	train->track_segment_ticks[13][50][68] = 40;
	train->track_segment_ticks[13][51][21] = 59;
	train->track_segment_ticks[13][52][69] = 54;
	train->track_segment_ticks[13][53][56] = 102;
	train->track_segment_ticks[13][53][73] = 96;
	train->track_segment_ticks[13][54][56] = 113;
	train->track_segment_ticks[13][54][73] = 105;
	train->track_segment_ticks[13][55][71] = 54;
	train->track_segment_ticks[13][56][75] = 53;
	train->track_segment_ticks[13][57][52] = 102;
	train->track_segment_ticks[13][57][55] = 113;
	train->track_segment_ticks[13][58][47] = 57;
	train->track_segment_ticks[13][59][74] = 39;
	train->track_segment_ticks[13][60][17] = 60;
	train->track_segment_ticks[13][61][72] = 102;
	train->track_segment_ticks[13][61][77] = 42;
	train->track_segment_ticks[13][62][28] = 32;
	train->track_segment_ticks[13][63][77] = 45;
	train->track_segment_ticks[13][64][29] = 73;
	train->track_segment_ticks[13][65][78] = 31;
	train->track_segment_ticks[13][66][48] = 34;
	train->track_segment_ticks[13][67][68] = 45;
	train->track_segment_ticks[13][68][53] = 60;
	train->track_segment_ticks[13][69][51] = 40;
	train->track_segment_ticks[13][69][66] = 45;
	train->track_segment_ticks[13][70][54] = 54;
	train->track_segment_ticks[13][71][45] = 127;
	train->track_segment_ticks[13][72][52] = 96;
	train->track_segment_ticks[13][73][60] = 102;
	train->track_segment_ticks[13][73][76] = 57;
	train->track_segment_ticks[13][74][57] = 53;
	train->track_segment_ticks[13][75][37] = 144;
	train->track_segment_ticks[13][75][58] = 39;
	train->track_segment_ticks[13][76][60] = 44;
	train->track_segment_ticks[13][76][62] = 45;
	train->track_segment_ticks[13][77][72] = 58;
	train->track_segment_ticks[13][78][43] = 54;
	train->track_segment_ticks[13][79][64] = 31;
	train->track_segment_ticks[14][0][44] = 79;
	train->track_segment_ticks[14][2][42] = 61;
	train->track_segment_ticks[14][2][44] = 96;
	train->track_segment_ticks[14][3][31] = 70;
	train->track_segment_ticks[14][4][38] = 61;
	train->track_segment_ticks[14][5][25] = 110;
	train->track_segment_ticks[14][6][27] = 80;
	train->track_segment_ticks[14][7][38] = 91;
	train->track_segment_ticks[14][8][23] = 49;
	train->track_segment_ticks[14][9][38] = 124;
	train->track_segment_ticks[14][10][38] = 158;
	train->track_segment_ticks[14][12][44] = 110;
	train->track_segment_ticks[14][15][44] = 141;
	train->track_segment_ticks[14][16][61] = 60;
	train->track_segment_ticks[14][17][40] = 53;
	train->track_segment_ticks[14][18][33] = 30;
	train->track_segment_ticks[14][19][40] = 60;
	train->track_segment_ticks[14][20][50] = 60;
	train->track_segment_ticks[14][21][43] = 51;
	train->track_segment_ticks[14][28][49] = 86;
	train->track_segment_ticks[14][28][65] = 78;
	train->track_segment_ticks[14][29][63] = 31;
	train->track_segment_ticks[14][30][2] = 66;
	train->track_segment_ticks[14][31][36] = 72;
	train->track_segment_ticks[14][31][41] = 55;
	train->track_segment_ticks[14][32][19] = 33;
	train->track_segment_ticks[14][33][49] = 74;
	train->track_segment_ticks[14][33][65] = 86;
	train->track_segment_ticks[14][35][37] = 141;
	train->track_segment_ticks[14][35][39] = 107;
	train->track_segment_ticks[14][36][46] = 51;
	train->track_segment_ticks[14][36][74] = 154;
	train->track_segment_ticks[14][37][30] = 72;
	train->track_segment_ticks[14][38][74] = 136;
	train->track_segment_ticks[14][40][30] = 56;
	train->track_segment_ticks[14][41][16] = 54;
	train->track_segment_ticks[14][41][18] = 56;
	train->track_segment_ticks[14][42][20] = 52;
	train->track_segment_ticks[14][42][79] = 54;
	train->track_segment_ticks[14][43][3] = 56;
	train->track_segment_ticks[14][44][70] = 149;
	train->track_segment_ticks[14][46][59] = 69;
	train->track_segment_ticks[14][48][32] = 85;
	train->track_segment_ticks[14][49][67] = 36;
	train->track_segment_ticks[14][50][68] = 42;
	train->track_segment_ticks[14][51][21] = 60;
	train->track_segment_ticks[14][52][69] = 58;
	train->track_segment_ticks[14][53][56] = 108;
	train->track_segment_ticks[14][53][73] = 95;
	train->track_segment_ticks[14][54][56] = 132;
	train->track_segment_ticks[14][54][73] = 119;
	train->track_segment_ticks[14][55][71] = 63;
	train->track_segment_ticks[14][56][75] = 54;
	train->track_segment_ticks[14][57][52] = 108;
	train->track_segment_ticks[14][59][74] = 48;
	train->track_segment_ticks[14][60][17] = 60;
	train->track_segment_ticks[14][61][72] = 106;
	train->track_segment_ticks[14][61][77] = 42;
	train->track_segment_ticks[14][62][28] = 36;
	train->track_segment_ticks[14][62][65] = 126;
	train->track_segment_ticks[14][63][77] = 42;
	train->track_segment_ticks[14][64][29] = 72;
	train->track_segment_ticks[14][64][63] = 126;
	train->track_segment_ticks[14][65][78] = 34;
	train->track_segment_ticks[14][66][48] = 37;
	train->track_segment_ticks[14][67][68] = 42;
	train->track_segment_ticks[14][68][53] = 60;
	train->track_segment_ticks[14][69][51] = 42;
	train->track_segment_ticks[14][69][66] = 49;
	train->track_segment_ticks[14][72][52] = 96;
	train->track_segment_ticks[14][73][60] = 106;
	train->track_segment_ticks[14][73][76] = 56;
	train->track_segment_ticks[14][74][57] = 54;
	train->track_segment_ticks[14][75][37] = 154;
	train->track_segment_ticks[14][76][60] = 42;
	train->track_segment_ticks[14][76][62] = 46;
	train->track_segment_ticks[14][77][72] = 60;
	train->track_segment_ticks[14][78][43] = 58;
	train->track_segment_ticks[14][79][64] = 30;
}
#else
static void init_track_b(struct train_profile* train, int track_tid) {

}
#endif
