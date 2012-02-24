#include <debug.h>
#include <string.h>
#include <syscall_kern.h>
#include <bwio.h>

#define RTC_VALUE_LOW  (unsigned int*)0x80810060
#define RTC_VALUE_HIGH (unsigned int*)0x80810064

#define TIMER_ENABLE 0x00000100

void rtc_init() {
	*RTC_VALUE_HIGH |= TIMER_ENABLE;
}

long rtc_get_ticks() {
	int low = *RTC_VALUE_LOW;
	return low / 983;
}

void kassert(unsigned int condition, char* fmt, ...) {
	if (condition == 0) {
		// Assertion failed
		char str[256];
		int length = 0;

	    va_list va;
		va_start(va,fmt);
	    sformat(&length, str, fmt, va);
	    va_end(va);
    	
		sys_assert(0, str);
	}
}
