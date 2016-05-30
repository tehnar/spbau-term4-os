#include "interrupt.h"
#include "stdio.h"

struct idt_ptr {
    uint16_t size;
    uint64_t base;
} __attribute__((packed));

typedef struct idt_ptr idt_ptr_t;

struct idt_descriptor {
    uint16_t offset_low;
    uint16_t segment_selector;
    uint8_t ist;
    uint8_t flags;    
    uint16_t offset_mid;
    uint32_t offset_high;
    uint32_t reserved;        
} __attribute__((packed));

typedef struct idt_descriptor idt_descriptor_t;

idt_descriptor_t idt_table[IDT_TABLE_SIZE];
idt_ptr_t idt_ptr;

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

void empty_handler(){};
void empty_pop_handler(){};
void syscall_handler(uint64_t type, uint64_t rsi);

WRAP_INTERRUPT(empty_handler);
WRAP_POP_INTERRUPT(empty_pop_handler);
WRAP_INTERRUPT(syscall_handler);

void idt_init() { 
    idt_ptr.size = sizeof(idt_descriptor_t) * IDT_TABLE_SIZE - 1;
    idt_ptr.base = (uint64_t) idt_table;
    __asm__ volatile ("lidt (%0)" : : "a"(&idt_ptr));
	for (int i = 0; i < IDT_TABLE_SIZE; i++)
		interrupt_init(i, &empty_handler_wrapper, INTERRUPT_INTGATE_TYPE);
	uint8_t error_interrupt_numbers[] = {8, 10, 11, 12, 13, 14, 17, 30};
	for (int i = 0; i < 8; i++)
		interrupt_init(error_interrupt_numbers[i], &empty_pop_handler_wrapper, INTERRUPT_INTGATE_TYPE);	

    interrupt_init(SYSCALL_INTERRUPT_NUMBER, &syscall_handler_wrapper, INTERRUPT_TRAP_TYPE | USER_PRIVILEGE); 
}

void syscall_handler(uint64_t type, uint64_t rsi) {
    switch (type) {
        case SYSCALL_WRITE:
            printf("%s", rsi);
            break; 
        default:
           DBG_ASSERT(0); // no other syscalls yet
    }    
}
