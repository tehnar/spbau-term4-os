#include "timer.h" 
#include "ioport.h"
#include "interrupt.h"

void timer_init(int rate, void *handler) {
    out8(PIT_CONTROL_PORT, PIT_INIT_WORD);    
    out8(PIT_ZERO_CHANNEL_DATA_PORT, rate & 255);
    rate >>= 8;
    out8(PIT_ZERO_CHANNEL_DATA_PORT, rate & 255);        
    interrupt_init(PIT_INTERRUPT, handler, INTERRUPT_INTGATE_TYPE); 
}