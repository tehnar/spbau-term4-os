#include "interrupt.h"
#include "print.h"

idt_descriptor_t idt_table[IDT_TABLE_SIZE];
idt_ptr_t idt_ptr;

extern void (*default_handler_wrapper);
extern void (*default_handler_err_wrapper);

void interrupt_init(int interrupt_num, void *handler_ptr, uint8_t type) {
    idt_descriptor_t entry;
    uint64_t ptr = (uint64_t) handler_ptr;
    entry.segment_selector = KERNEL_CODE;
    entry.offset_low  = ptr & 0xffff;
    entry.offset_mid  = (ptr>>16) & 0xffff;
    entry.offset_high = (ptr>>32);
    entry.ist = 0;
    entry.reserved = 0;
    entry.flags = type | INTERRUPT_PRESENT_FLAG;
    idt_table[interrupt_num] = entry;    
}

void idt_init() { 
    idt_ptr.size = sizeof(idt_descriptor_t) * IDT_TABLE_SIZE - 1;
    idt_ptr.base = (uint64_t) idt_table;
    __asm__ volatile ("lidt (%0)" : : "a"(&idt_ptr));
}
