#include <ts7200.h>
#include <timer.h>

#define TIMER3_LOAD ((unsigned int *)TIMER3_BASE)
#define TIMER3_VALUE ((unsigned int *)(TIMER3_BASE + VAL_OFFSET))
#define TIMER3_CONTROL ((unsigned int *)(TIMER3_BASE + CRTL_OFFSET))

#define TIMER2_LOAD ((unsigned int *)TIMER2_BASE)
#define TIMER2_VALUE ((unsigned int *)(TIMER2_BASE + VAL_OFFSET))
#define TIMER2_CONTROL ((unsigned int *)(TIMER2_BASE + CRTL_OFFSET))

void timer3_init() {
	// disable timer so we can modify it
	*TIMER3_CONTROL &= ~ENABLE_MASK;
	
	// Clear the timer settings
	*TIMER3_CONTROL = 0;
	
	// set clock to 2khz (2000 cycles per second)
	*TIMER3_CONTROL &= ~CLKSEL_MASK;
	
	// set the clock to underflow every 100 ms
	*TIMER3_LOAD = 0xffffffff;
	
	// set clock to freerun
	*TIMER3_CONTROL &= ~MODE_MASK;
	
	// Start the timer
	*TIMER3_CONTROL |= ENABLE_MASK;
}

void timer2_init() {
	// disable timer so we can modify it
	*TIMER2_CONTROL &= ~ENABLE_MASK;
	
	// Clear the timer settings
	*TIMER2_CONTROL = 0;
	
	// set clock to 2khz (2000 cycles per second)
	*TIMER2_CONTROL &= ~CLKSEL_MASK;
	
	// set the clock to underflow every 100 ms
	*TIMER2_LOAD = 20;
	
	// set clock to preload (wrap around to TIMER3_LOAD after
	// underflow
	*TIMER2_CONTROL |= MODE_MASK;
	
	// Start the timer
	*TIMER2_CONTROL |= ENABLE_MASK;// disable timer so we can modify it
}

void timer2_disable() {
	*TIMER2_CONTROL &= ~ENABLE_MASK;
}

void timer3_disable() {
	*TIMER3_CONTROL &= ~ENABLE_MASK;
}
	
