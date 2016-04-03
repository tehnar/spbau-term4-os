.code64
.section .text
.globl switch_threads
.globl switch_threads_and_start 

switch_threads:
    pushfq    
    pushq %rbp        // save volatile registers
    pushq %rbx
    pushq %r12
    pushq %r13
    pushq %r14
    pushq %r15

    movq %rsp, (%rdi) // save SP
    movq %rsi, %rsp   // restore SP

    popq %r15         // restore volatile register
    popq %r14
    popq %r13
    popq %r12
    popq %rbx
    popq %rbp
    popfq
    callq finish_thread
    //sti will be enabled after return from yield
    cld
    ret

switch_threads_and_start:
    pushfq
    pushq %rbp        // save volatile registers
    pushq %rbx
    pushq %r12
    pushq %r13
    pushq %r14
    pushq %r15

    movq %rsp, (%rdi) // save SP
    movq %rsi, %rsp   // restore SP

    popq %r15         // restore volatile register
    popq %r14
    popq %r13
    popq %r12
    popq %rbx
    popq %rbp
    popfq 
    cld
    sti // was locked in start_thread
    popq %rdi         // void* arg
    ret

