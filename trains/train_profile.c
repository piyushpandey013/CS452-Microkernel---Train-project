#include <train_profile.h>
#include <track_server.h>
#include <track_util.h>
#include <string.h>

#include <clock.h>

void train_profile_init(struct train_profile* train, int num) {
	train->num = num;
	train->tid = -1;
	train->length = 0;
	train->direction = DIRECTION_STRAIGHT;

	train->latest_sensor_node = -1;
	train->latest_sensor_timestamp = 0;

	train->current_speed = 0;
	train->speed_index = 0;

	train->stop_location.node = -1;
	train->stop_location.offset = 0;

	train->route.cursor = -1;
	train->route.rev_cursor = -1;
	train->route.reply_tid = -1;

	train->paused = 0;
	train->previous_speed_index = 0;

	train->accelerating_position_func = 0;
	train->accelerating_velocity_func = 0;
	train->stopping_distance_func = 0;

	memset(&train->snapshot, 0, sizeof(train->snapshot));

	memset(train->track_segment_ticks, 0, sizeof(train->track_segment_ticks));
	memset(train->track_segment_ticks_freq, 0, sizeof(train->track_segment_ticks_freq));
	memset(train->stopping_distance, 0, sizeof(train->stopping_distance));
}

void train_profile_fill_with_average_ticks(struct train_profile* train, int track_tid) {

	struct track_node *track = get_track();

	// Calculate the average velocity for each speed
	int speed;
	for (speed = 1; speed <= 14 + 13; speed++) {
		int average_velocity = 0;
		int samples = 0;

		int i, j;
		for (i = A1; i <= E16; i++) {
			for (j = A1; j <= E16; j++) {

				int ticks = train->track_segment_ticks[speed][i][j];
				if (ticks > 0) {
					struct track_node *i_node = &(track[i]);
					struct track_node *j_node = &(track[j]);

					average_velocity += (dist_between_adjacent_sensors(i_node->edge[DIR_AHEAD].dest, j_node, i_node->edge[DIR_AHEAD].dist) * TICKS_PER_SEC) / ticks;
					samples++;
				}
			}
		}

		if (samples == 0) {
			train->avg_velocities[speed] = 0;
		} else {
			train->avg_velocities[speed] = average_velocity / samples;
		}
	}

	// Fill in the missing edges with either reverse track velocity or average velocity
	for (speed = 1; speed <= 14 + 13; speed++) {
		int i, j;

		for (i = A1; i <= E16; i++) {
			for (j = A1; j <= E16; j++) {
				train->track_segment_ticks_freq[speed][i][j] = 1;

				// i->j is the edge we're describing
				// to get the reverse edge: j_reverse -> i_reverse
				int j_rev = track[j].reverse->num;
				int i_rev = track[i].reverse->num;;

				int ticks = train->track_segment_ticks[speed][i][j];
				int rev_ticks = train->track_segment_ticks[speed][j_rev][i_rev];
				if (ticks == 0) {
					if (rev_ticks > 0) {
						train->track_segment_ticks[speed][i][j] = rev_ticks;
					} else {
						struct track_node *i_node = &(track[i]);
						struct track_node *j_node = &(track[j]);

						if (train->avg_velocities[speed] == 0) {
							train->track_segment_ticks[speed][i][j] = 0;
						} else {
							train->track_segment_ticks[speed][i][j] = (dist_between_adjacent_sensors(i_node->edge[DIR_AHEAD].dest, j_node,  i_node->edge[DIR_AHEAD].dist) * TICKS_PER_SEC) / train->avg_velocities[speed];
						}
					}
				}
			}
		}
	}
}
