#ifndef __CLOCK_H__
#define __CLOCK_H__

#define CLOCK_SERVER "clock_server"

int Delay(int ticks, int tid);
int DelayUntil(int ticks, int tid);
int Time(int tid);

#define CLOCK_NOTIFIER 1
#define CLOCK_DELAY 2
#define CLOCK_DELAY_UNTIL 3
#define CLOCK_TIME 4

#define TICKS_PER_SEC 100
#define TICKS_PER_MSEC 10

#define MSEC /(10)
#define SEC  *(100)
#endif
