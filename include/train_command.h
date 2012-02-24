#ifndef __TRAIN_COMMAND_H__
#define __TRAIN_COMMAND_H__

#define NUM_SENSOR_MODULES 5
#define NUM_SENSORS_PER_MODULE 16

#define IS_SWITCH(sw) ((sw >= 1 && sw <= 18) || (sw >= 0x9A && sw <= 0x9C) || sw == 0x99)
#define IS_SWITCH_NODE(node) (((node) >= 80 && (node) <= 122) && ((node)%2 == 0))

#define IS_BRANCH(node) (((node) >= 80 && (node) <= 122) && ((node)%2 == 0))
#define IS_MERGE(node) (((node) >= 81 && (node) <= 123) && ((node)%2 == 1))

#define IS_TURNOUT(node) ((node) >= 80 && (node) <= 123)

int TrainCommandGo(int tid);
int TrainCommandStop(int tid);
int TrainCommandSpeed(char train, char speed, int tid);
int TrainCommandReverse(char train, int tid);
int TrainCommandResetSensors(int tid);
int TrainCommandReadSensors(int modules, int tid);
int TrainCommandSwitchCurved(int sw, int tid);
int TrainCommandSwitchStraight(int sw, int tid);
int TrainCommandSolenoidOff(int tid);

#define TRAIN_COMMAND "train_command"
#endif
