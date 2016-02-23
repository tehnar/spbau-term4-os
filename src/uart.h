#ifndef __UART_H__
#define __UART_H__

#define UART_DATA_REGISTER_PORT 0x3f8
#define UART_INTERRUPT_ENABLE_REGISTER_PORT 0x3f9
#define UART_LINE_CONTROL_REGISTER_PORT 0x3fb
#define UART_DATA_REGISTER_PORT_MODE 0b10000000 //if set then 0x3f8 and 0x3f9 will be used as a divisor coefficients
#define UART_LINE_STATUS_REGISTER 0x3fd
#define UART_IS_READY_BIT 0b00100000
#define UART_EIGHT_FRAME_BITS 0b00000011

void uart_init();

int uart_is_ready();

int putc(char c);

#endif /* __UART_H__ */
