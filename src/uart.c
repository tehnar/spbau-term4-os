#include "uart.h"
#include "ioport.h"

void uart_init() {
    out8(UART_LINE_CONTROL_REGISTER_PORT, UART_DATA_REGISTER_PORT_MODE);
    out8(UART_DATA_REGISTER_PORT, 1);
    out8(UART_INTERRUPT_ENABLE_REGISTER_PORT, 0);

    out8(UART_LINE_CONTROL_REGISTER_PORT, UART_EIGHT_FRAME_BITS);    
    out8(UART_INTERRUPT_ENABLE_REGISTER_PORT, 0);
}

inline int uart_is_ready() {
    return in8(UART_LINE_STATUS_REGISTER) & UART_IS_READY_BIT; 
}

int putc(char c) {
    while (!uart_is_ready());
    out8(UART_DATA_REGISTER_PORT, c);
    return c;
}
