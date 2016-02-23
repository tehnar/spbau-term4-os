#ifndef __PIC_H__
#define __PIC_H__

#include "ioport.h"

#define PIC_MASTER_COMMAND_REGISTER_PORT 0x20
#define PIC_MASTER_DATA_REGISTER_PORT 0x21
#define PIC_SLAVE_COMMAND_REGISTER_PORT 0xA0
#define PIC_SLAVE_DATA_REGISTER_PORT 0xA1

#define PIC_INIT_WORD 0b00010001
#define PIC_MASTER_INTERRUPT_OFFSET 0x20
#define PIC_SLAVE_INTERRUPT_OFFSET 0x28
#define PIC_SLAVE_PIN 2
#define PIC_MODE 1

#define PIC_EOI 0b00100000

void pic_init();

void pic_eoi(int is_slave);

#endif /* __PIC_H__ */
