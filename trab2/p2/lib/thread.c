#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>

#include <queue.h>
#include <thread.h>
#include <threadu.h>

node_t *ready_queue;
tcb_t *current_running = NULL; // Currently running thread

int tid_global = 0;

/*
  TODO:  thread_init: initializes  the  thread library  and creates  a
  thread for  the main function. Returns  0 on success, or  -EINVAL if
  the library has already been initialized.
 */
int thread_init()
{
	// Check if the library has already been initialized
    if (!is_empty(ready_queue) || current_running != NULL) {
        return -EINVAL;
    }

    // Initialize the ready queue
    queue_init(&ready_queue);

    // Create a thread for the main function
    tcb_t *main_thread = (tcb_t *)malloc(sizeof(tcb_t));
    if (main_thread == NULL) {
        return -ENOMEM; // Memory allocation failed
    }
    main_thread->tid = tid_global++;
    enqueue(&ready_queue, main_thread);

    return 0; // Success
}

// TODO: creates a new thread and inserts it in the ready queue.
int thread_create(thread_t *thread, void *(*start_routine)(void *), void *arg)
{
	// Check if thread or start_routine is NULL
    if (thread == NULL || start_routine == NULL) {
        return -EINVAL;
    }

    // Allocate memory for the new thread control block (TCB)
    tcb_t *new_thread = (tcb_t *)malloc(sizeof(tcb_t));
    if (new_thread == NULL) {
        return -ENOMEM; // Memory allocation failed
    }

    // Set thread attributes
    new_thread->tid = tid_global++;
    new_thread->start_routine = start_routine;
    new_thread->arg = arg;

    // Insert the new thread into the ready queue
    enqueue(&ready_queue, new_thread);

    // Assign the thread ID to the output parameter
    thread->tcb->tid = new_thread->tid;

    return 0; // Success
}

// TODO: yields the CPU to another thread
int thread_yield() {
    // Check if there's a currently running thread
    if (current_running == NULL) {
        return 1; // No thread is running
    }

    // Move the current thread to the end of the ready queue
    if (current_running->status == READY) {
        enqueue(&ready_queue, current_running);
    }

    // Call scheduler_entry to select the next thread to run
    scheduler_entry();

    return 0; // Success
}

// TODO: waits for a thread to finish
int thread_join(thread_t *thread, int *retval) {
    // Find the TCB associated with the specified thread ID
    tcb_t *target_thread_tcb = thread->tcb;
	if (target_thread_tcb == NULL) {
        return 1; // Thread not found
    }

    // Check if the specified thread ID is valid
    if (target_thread_tcb == NULL || target_thread_tcb->tid < 0 || target_thread_tcb->tid >= tid_global) {
        return 1; // Invalid thread ID
    }

    // Wait for the specified thread to finish execution
    while (target_thread_tcb->status != EXITED) {
        sleep(1);
    }

    // Optionally retrieve the return value of the specified thread
	if (target_thread_tcb->status == EXITED) {
    	*retval = 0;
	} else {
		*retval = 1;
	}

    free(target_thread_tcb);

    return 0; // Success
}

// TODO: marks a thread as EXITED and terminates the thread
void thread_exit(int status)
{
    // Mark the current thread's status as EXITED
    current_running->status = EXITED;
    current_running->return_value = status; // Store the exit status

    // Call the scheduler to switch to the next runnable thread
    scheduler_entry();
}


// TODO: selects the next thread to execute
void scheduler_entry()
{
	if (current_running == NULL) {
        return; // No threads to schedule
    }

    // Find the next READY thread
    tcb_t *next_thread = current_running->next;
    while (next_thread != NULL && next_thread->status != READY) {
        next_thread = next_thread->next;
    }

    // If no READY threads are found, start from the head of the list
    if (next_thread == NULL) {
        next_thread = ready_queue->head;
        while (next_thread != NULL && next_thread->status != READY) {
            next_thread = next_thread->next;
        }
    }

    // If a READY thread is found, switch to it
    if (next_thread != NULL) {
        current_running = next_thread;
    } else {
        // No READY threads found, system idle or handle accordingly
        printf("System idle...\n");
    }
}

// TODO: you must  make sure this function is called if a thread does
// not call thread_exit
void exit_handler()
{
	if (current_running == NULL) {
        return; // No thread to exit
    }

    // Mark the current thread as EXITED
    current_running->status = EXITED;

    // Perform cleanup tasks, e.g., deallocate stack
    free(current_running->stack_base);

    // Trigger the scheduler to select a new thread to run
    scheduler_entry();
}
