#ifndef __DEBUG_H__
#define __DEBUG_H__

#define KASSERT(c, f, ...)	kassert(c, f, __VA_ARGS__)

void rtc_init();
long rtc_get_ticks();
void kassert(unsigned int condition, char* format, ...);

#endif

