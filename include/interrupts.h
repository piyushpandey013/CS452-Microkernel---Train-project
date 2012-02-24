#ifndef __INTERRUPTS_H__
#define __INTERRUPTS_H__

#include <task.h>

#define INT_TYPE_HWI 1
#define INT_TYPE_SWI 0

#define HWI_CONTINUE 0
#define HWI_PREEMPT 1

void interrupts_init();
void interrupts_enable(int eventId);
void interrupts_disable_all();
int interrupts_handle_irq(struct task *t);

#endif
