#ifndef __INTERRUPT_H__
#define __INTERRUPT_H__

#include <stdint.h>
#include "memory.h"

#define IDT_TABLE_SIZE 256
#define INTERRUPT_INTGATE_TYPE 0b1110
#define INTERRUPT_PRESENT_FLAG 0b10000000

#define PUSHAD "\
    push %rax; \
    push %rcx; \
    push %rdx; \
    push %r8;  \
    push %r9;  \
    push %r10; \
    push %r11; "

#define POPAD "\
    pop %r11; \
    pop %r10; \
    pop %r9;  \
    pop %r8;  \
    pop %rdx; \
    pop %rcx; \
    pop %rax; "

#define WRAP_INTERRUPT(func) void func##_wrapper();\
    __asm__( \
    #func"_wrapper: "\
    PUSHAD \
    "cld; \
    call " #func ";" \
    POPAD \
    "iretq;");  

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

static inline void interrupt_enable()
{ __asm__ volatile ("sti"); }

static inline void interrupt_disable()
{ __asm__ volatile ("cli"); }


extern idt_descriptor_t idt_table[IDT_TABLE_SIZE];

void idt_init();

void interrupt_init(int interrupt_num, void *handler_ptr, uint8_t type);

#endif /*__INTERRUPT_H__*/
