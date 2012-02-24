#ifndef __TRAIN_H__
#define __TRAIN_H__

int CreateTrainTask(int priority, int train);

int TrainSpeed(int speed, int tid);
int TrainStop(int delay, int tid);
int TrainReverse(int old_speed, int tid);

#endif /* TRAIN_H_ */
