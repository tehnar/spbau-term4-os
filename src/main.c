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

void test_runner_simple(void *args) {
    int arg =  *((int*)args);
    printf("Started processing of thread %d\n", arg);
    for (int i = 0; i < 5; i++) {
        printf("Thread %d, iteration %d\n", arg, i);
        yield();
    }
}

#define TEST_THREAD_COUNT 10
void test_threads_simple() {
    int args[TEST_THREAD_COUNT];
    pid_t threads[TEST_THREAD_COUNT];
    for (int i = 0; i < TEST_THREAD_COUNT; i++) {
        args[i] = i;
        threads[i] = thread_create(&test_runner_simple, (void*) &args[i]);    
        printf("Created thread with pid %d\n", threads[i]);
    }

    for (int i = 0; i < TEST_THREAD_COUNT; i++) 
        thread_start(threads[i]);
    for (int i = 0; i < TEST_THREAD_COUNT; i++) {
        thread_join(threads[i], NULL);
        printf("Thread %d (pid %d) joined\n", i, threads[i]);
    }
    puts("Test simple finished");
}
#undef TEST_THREAD_COUNT

uint64_t calc() {
    uint64_t sum = 0;
    for (int i = 0; i < (int) 1e8; i++) {
        sum += i;
    }
    return sum;
}

spinlock_t l;
void func1(void *args) {
    int* state_ptr = (int*) args;
    lock(&l);
    puts("func1");
    DBG_ASSERT(*state_ptr == 0);
    uint64_t sum = calc();
    DBG_ASSERT(*state_ptr == 0);
    printf("func1 finished, sum=%llu\n", sum);
    *state_ptr = 1;
    unlock(&l);
    thread_exit(sum);
}


void func2(void *args) {
     int* state_ptr = (int*) args;
     DBG_ASSERT(*state_ptr == 0);
     lock(&l);
     puts("func2");
     DBG_ASSERT(*state_ptr == 1);
     uint64_t sum = calc();
     DBG_ASSERT(*state_ptr == 1);
     printf("func2 finished, sum=%llu\n", sum);
     unlock(&l);
     thread_exit(sum);
}

void test_threads_lock() {
    create_spinlock(&l);
    int state = 0;
    pid_t t1 = thread_create(&func1, (void*) &state), t2 = thread_create(&func2, (void*) &state);
    thread_start(t1);        
    thread_start(t2);
    uint64_t val = 0;
    thread_join(t1, (void**) &val);
    DBG_ASSERT(val == 4999999950000000);
    thread_join(t2, (void**) &val);
    DBG_ASSERT(val == 4999999950000000);
    printf("Threads joined (pids %d, %d)!\n", t1, t2);
}

void test_threads_binary_tree(void *arg) {
    long long depth = (long long) arg;
    printf("Enter test binary tree, depth=%lld, thread id=%d\n", depth, get_current_thread());
    if (!depth) {
        thread_exit(1);
        DBG_ASSERT(0);
    }
    calc();
    pid_t l = thread_create(&test_threads_binary_tree, (void*) (depth - 1));
    pid_t r = thread_create(&test_threads_binary_tree, (void*) (depth - 1));
    thread_start(l);
    thread_start(r);
    int x = 0, y = 0;
    thread_join(r, (void**) &x);
    thread_join(l, (void**) &y);
    printf("Thread %d at depth %lld finished with %d\n", get_current_thread(), depth, x + y);
    thread_exit(x + y);
}
void main(void) { 
    pic_init();      
    uart_init();
    idt_init();
    setup_misc();
    setup_memory();
    setup_buddy();
    setup_paging();
    setup_alloc();
    threads_init();
    interrupt_enable();

    test_threads_simple();
    test_threads_lock();
    test_threads_binary_tree((void*) 4);
    puts("All test completed!");

    while (1);
}

