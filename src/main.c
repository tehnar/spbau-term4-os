#include "pic.h"
#include "interrupt.h"
#include "uart.h"
#include "timer.h"
#include "stdio.h"
#include "misc.h"
#include "memory.h"
#include "balloc.h"
#include "paging.h"
#include "kmem_cache.h"
#include "threads.h"
#include "fs.h"
#include "initramfs.h"
#include "string.h"
#include "elf.h"


void main(void) { 
    pic_init();      
    uart_init();
    idt_init();
    setup_misc();
    setup_memory();
    setup_initramfs();        
    setup_buddy();
    setup_paging();
    setup_alloc();
    threads_init();
    interrupt_enable();
    init_fs();
    read_initramfs();
    
    run_elf("/test/main");

    puts("Tests completed");
    while (1);
}

