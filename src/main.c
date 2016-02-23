#include "pic.h"
#include "interrupt.h"
#include "uart.h"
#include "timer.h"
#include "print.h"

void interrupt_timer_handler() { 
    static int counter = 0;
    counter++;
    if (counter * PIT_MAX_RATE >= PIT_FREQUENCY) {
        counter = 0;
        puts("Hello, OS!");                     
    }
    pic_eoi(0);                       
}

WRAP_INTERRUPT(interrupt_timer_handler);

void main(void) { 
    idt_init();
    uart_init();
    pic_init();      
    puts("initing timer");
    timer_init(PIT_MAX_RATE, &interrupt_timer_handler_wrapper);
    puts("started!");
    printf("Actually, last string contained %d chars\n", printf("Characters: %c %c and numbers: %d %u %x %o\n", 'a', 65, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff));
    printf("%s\n", "A string");                                                                                     
    printf("%d%lld test hex:%x %llx %p\n", 123, 123456789123456789ll, 123456, (uint64_t) &main, &main);
    char buf[100] = {0};
    snprintf(buf, 55, "Truncated address of timer interrupt wrapper: %p\n", &interrupt_timer_handler_wrapper);
    puts(buf);
    interrupt_enable();
    while (1);
}

