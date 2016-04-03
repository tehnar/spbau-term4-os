#include "threads.h"
#include "interrupt.h"
#include "kmem_cache.h"
#include "stdio.h"
#include "timer.h"

typedef enum {NOT_STARTED, RUNNING, FINISHED, WAIT_FOR_JOIN} thread_state;
struct thread {
    uint64_t *rsp, *rsp_start;
    uint64_t exit_code;
    thread_state state;
};

typedef struct thread thread_t;

static volatile thread_t threads[MAX_THREADS];
static volatile pid_t threads_head;
static volatile pid_t threads_next[MAX_THREADS];
static volatile pid_t threads_prev[MAX_THREADS];

static volatile pid_t current_thread = 0, previous_thread = 0;

void switch_threads(void **old_sp, void *new_sp);
void switch_threads_and_start(void **old_sp, void *new_sp);

void create_spinlock(spinlock_t* lock) {
    lock->users = 0;
    lock->ticket = 0;
}

void lock(spinlock_t *lock) {
    uint16_t ticket = __sync_fetch_and_add(&lock->users, 1);

    while (lock->ticket != ticket) {
        barrier();
        yield();
    }
     __sync_synchronize();
}

void unlock(spinlock_t *lock) {
    __sync_synchronize();
    __sync_add_and_fetch(&lock->ticket, 1);   
} 

void thread_exit_with_no_code() {
    thread_exit(0);
}

void schedule() {
    static int counter = 0;
    counter++;
    if (counter * PIT_MAX_RATE * 100 >= PIT_FREQUENCY) { // ~10 ms per thread run
        counter = 0;
        pic_eoi(0);
        yield();
    } else {
        pic_eoi(0);
    }
}

WRAP_INTERRUPT(schedule);

void threads_init() {
    threads[0].state = RUNNING;
    for (int i = 1; i < MAX_THREADS - 1; i++) {
        threads_next[i] = i + 1;
        threads_prev[i + 1] = i;
    }
    threads_next[0] = 0;
    threads_prev[0] = 0;
    threads_next[MAX_THREADS - 1] = 1;
    threads_prev[1] = MAX_THREADS - 1;
    threads_head = 1;
    timer_init(PIT_MAX_RATE, &schedule_wrapper);
}


pid_t thread_create(void (*fptr) (void *), void *arg) {
    interrupt_disable();
    pid_t new_thread_id = threads_head; 
    
    threads_head = threads_next[threads_head];

    // erase thread from list of free threads
    threads_prev[threads_head] = threads_prev[threads_prev[threads_head]];
    threads_next[threads_prev[threads_head]] = threads_head;

    
    volatile thread_t *new_thread = &threads[new_thread_id];
    new_thread->rsp_start = (uint64_t*) kmem_alloc(THREAD_STACK_SIZE);
    new_thread->rsp = new_thread->rsp_start + THREAD_STACK_SIZE;
    *(--new_thread->rsp) = (uint64_t) &thread_exit_with_no_code;
    *(--new_thread->rsp) = (uint64_t) fptr; 
    *(--new_thread->rsp) = (uint64_t) arg;
    *(--new_thread->rsp) = 0; // rflags
    *(--new_thread->rsp) = 0; // rbx 
    *(--new_thread->rsp) = 0; // rbp 
    *(--new_thread->rsp) = 0; // r12 
    *(--new_thread->rsp) = 0; // r13 
    *(--new_thread->rsp) = 0; // r14 
    *(--new_thread->rsp) = 0; // r15 
    interrupt_enable();
    return new_thread_id;
}

void finish_thread() {
    if (threads[previous_thread].state == FINISHED) {
        threads[previous_thread].state = WAIT_FOR_JOIN;
        kmem_free(threads[previous_thread].rsp_start);
    }
}

static void thread_run(pid_t thread_id) {
    if (thread_id != current_thread) {
        previous_thread = current_thread;
        current_thread = thread_id;
        switch_threads((void**) &threads[previous_thread].rsp, (void*) threads[current_thread].rsp);
    }
}


void thread_start(pid_t thread_id) {
    interrupt_disable();
    previous_thread = current_thread;
    current_thread = thread_id;
    threads[current_thread].state = RUNNING;
    
    // add thread to list of active threads
    threads_next[thread_id] = 0;
    threads_prev[thread_id] = threads_prev[0];
    threads_next[threads_prev[0]] = thread_id;
    threads_prev[0] = thread_id;

    switch_threads_and_start((void**) &threads[previous_thread].rsp, (void*) threads[current_thread].rsp);
    interrupt_enable();
}

pid_t get_current_thread() {
    return current_thread;
}

void yield() {
    interrupt_disable();
    thread_run(threads_next[current_thread]);
    interrupt_enable();
}

void thread_join(pid_t thread_id, void **retval) {
    while (threads[thread_id].state != WAIT_FOR_JOIN) {
        yield();
    }
    interrupt_disable();
    //add thread to list of free threads
    threads_next[thread_id] = threads_head;
    threads_prev[thread_id] = threads_prev[threads_head];
    threads_next[threads_prev[threads_head]] = thread_id;
    threads_prev[threads_head] = thread_id;
    threads[thread_id].state = NOT_STARTED;
    if (retval) {
        *retval = (void*) threads[thread_id].exit_code;
    }
    interrupt_enable();
}

void thread_exit(uint64_t code) {   
    interrupt_disable();
    threads[current_thread].state = FINISHED;
    threads[current_thread].exit_code = code;
    // erase thread from list of active threads
    threads_next[threads_prev[current_thread]] = threads_next[current_thread];
    threads_prev[threads_next[current_thread]] = threads_prev[current_thread];
    yield(); 
}   
