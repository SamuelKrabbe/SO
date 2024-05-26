#ifndef THREAD_H
#define THREAD_H

#include <stdint.h>
#include <threadu.h>

#define NUMBER_OF_REGISTERS	15
#define STACK_SIZE		2048

void scheduler_entry();
void scheduler();
void exit_handler();

typedef enum {
	      FIRST_TIME,
	      READY,
	      BLOCKED,
	      EXITED
} status_t;

typedef struct tcb {
    int tid;                           // Thread ID
    uint64_t *stack_pointer;           // Pointer to the saved stack
    void *(*start_routine)(void *);    // Thread's start routine
    void *arg;                         // Argument to the start routine
    int exit_status;                   // Exit status
    status_t thread_status;            // Current status of the thread
	uint64_t cpu_time;                 // CPU time of the thread
    struct tcb *next;                  // Pointer to the next TCB in the ready queue
} tcb_t;


#endif /* THREAD_H */
