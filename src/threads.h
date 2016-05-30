#ifndef THREADS_H
#define THREADS_H

#include <stdint.h>
#include <sys/types.h>
#include "memory.h"

#define MAX_THREADS (1<<16)
#define THREAD_STACK_SIZE (1<<13)

static inline void barrier() {
    __asm__ volatile("": : :"memory");
}

typedef enum {NOT_STARTED, RUNNING, FINISHED, WAIT_FOR_JOIN} thread_state;
struct thread {
    uint64_t *rsp, *rsp_start;
    uint64_t exit_code;
    thread_state state;
};

typedef struct thread thread_t;

struct spinlock {
    uint16_t users;
    uint16_t ticket;    
};

typedef struct spinlock spinlock_t;

void lock(spinlock_t *lock);
void unlock(spinlock_t *lock);

void init_threads();

pid_t thread_create(void (*fptr)(void *), void *arg);

void thread_start(pid_t tid);

void yield();
void thread_exit(uint64_t code);
void threads_init();
void thread_join(pid_t thread_id, void **retval);
thread_t* get_current_thread();

void create_spinlock(spinlock_t* lock);

#endif /* THREADS_H */
