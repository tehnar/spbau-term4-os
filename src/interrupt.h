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
    push %rdi; \
    push %rsi; \
    push %r8;  \
    push %r9;  \
    push %r10; \
    push %r11; "

#define POPAD "\
    pop %r11; \
    pop %r10; \
    pop %r9;  \
    pop %r8;  \
    pop %rsi; \
    pop %rdi; \
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

#define WRAP_POP_INTERRUPT(func) void func##_wrapper();\
    __asm__( \
    #func"_wrapper: "\
    PUSHAD \
    "cld; \
    call " #func ";" \
    "add $8, %rsp;" \
    POPAD \
    "iretq;");  

static inline void interrupt_enable()
{ __asm__ volatile ("sti" : : : "cc"); }
//{ __asm__ volatile ("sti"); }

static inline void interrupt_disable()
{ __asm__ volatile ("cli" : : : "cc"); }
//{ __asm__ volatile ("cli"); }

void idt_init();

void interrupt_init(int interrupt_num, void *handler_ptr, uint8_t type);

#endif /*__INTERRUPT_H__*/
