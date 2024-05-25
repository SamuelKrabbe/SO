#ifndef THREAD_H
#define THREAD_H

#include <stdint.h>

#define NUMBER_OF_REGISTERS	15
#define STACK_SIZE		2048

void scheduler_entry();
void exit_handler();

typedef enum {
	      FIRST_TIME,
	      READY,
	      BLOCKED,
	      EXITED
} status_t;

// Function pointer type for thread start routine
typedef void *(*start_routine_t)(void *);

typedef struct tcb {
	int tid;                          // Unique identifier for the thread
    void *registers[NUMBER_OF_REGISTERS]; // Array to store thread's registers
    void *stack_pointer;              // Pointer to the current position in the stack
    void *stack_base;                 // Base of the thread's stack
    int priority;                     // Priority of the thread for scheduling
    unsigned int execution_time;      // Execution time of the thread
    status_t status;                  // Current status of the thread
    start_routine_t start_routine;    // Start routine of the thread
    void *arg;          // Argument to be passed to the start routine
    int return_value;
    struct tcb *next; 
} tcb_t;

#endif /* THREAD_H */
