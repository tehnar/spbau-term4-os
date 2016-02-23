#ifndef __TIMER_H__
#define __TIMER_H__

#include "pic.h"

#define PIT_PIN 0
#define PIT_CONTROL_PORT 0x43
#define PIT_ZERO_CHANNEL_DATA_PORT 0x40
#define PIT_FREQUENCY 1193180
#define PIT_MAX_RATE ((1<<16) - 1)
#define PIT_INIT_WORD 0b00110100
#define PIT_INTERRUPT (PIC_MASTER_INTERRUPT_OFFSET + 0)

void timer_init(int frequency, void *handler);   
#endif /* __TIMER_H__ */
