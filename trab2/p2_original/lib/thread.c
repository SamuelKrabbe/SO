#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <queue.h>
#include <thread.h>

node_t *ready_queue;
tcb_t *current_running;

int tid_global = 0;

void debug_print_current_running() {
    char *status[] = {"FIRST_TIME", "READY", "BLOCKED", "EXITED"};
    if (current_running != NULL) {
        printf("Current Running Thread ID: %d, Status: %s, CPU Time: %lu\n",
               current_running->tid,
               status[current_running->thread_status],
               current_running->cpu_time);
    } else {
        printf("No current running thread.\n");
    }
}

int thread_init() {
    if (ready_queue != NULL) {
        return -EINVAL;  // Already initialized
    }

    queue_init(&ready_queue);

    // Initialize main thread
    current_running = (tcb_t *)malloc(sizeof(tcb_t));
    if (current_running == NULL) {
        return -ENOMEM;
    }

    current_running->tid = tid_global++;
    current_running->stack_pointer = NULL;
    current_running->start_routine = NULL;
    current_running->arg = NULL;
    current_running->exit_status = 0;
    current_running->thread_status = READY;
    current_running->cpu_time = 0;
    current_running->next = NULL;

    return 0;
}

int thread_create(thread_t *thread, void *(*start_routine)(void *), void *arg) {
    tcb_t *new_tcb = (tcb_t *)malloc(sizeof(tcb_t));
    if (new_tcb == NULL) {
        return -ENOMEM;
    }

    new_tcb->tid = tid_global++;
    new_tcb->stack_pointer = (uint64_t *)malloc(STACK_SIZE);
    if (new_tcb->stack_pointer == NULL) {
        free(new_tcb);
        return -ENOMEM;
    }
    new_tcb->start_routine = start_routine;
    new_tcb->arg = arg;
    new_tcb->exit_status = 0;
    new_tcb->thread_status = FIRST_TIME;
    new_tcb->cpu_time = 0;
    new_tcb->next = NULL;

    thread->tcb = new_tcb;

    // Add the new thread to the ready queue
    node_t *new_node = (node_t *)malloc(sizeof(node_t));
    if (new_node == NULL) {
        free(new_tcb->stack_pointer);
        free(new_tcb);
        return -ENOMEM;
    }

    new_node->thread = new_tcb;
    new_node->next = NULL;
    enqueue(&ready_queue, new_node);

    return 0;
}

int thread_yield() {
	// print_queue(ready_queue);
    // Add the current thread to the ready queue if it's still ready
    if (current_running->thread_status == READY) {
        node_t *node = (node_t *)malloc(sizeof(node_t));
        node->thread = current_running;
        enqueue(&ready_queue, node);
    }

    // Call the scheduler to select the next thread
    scheduler_entry();

    return 0;
}

int thread_join(thread_t *thread, int *retval) {
    tcb_t *tcb = (tcb_t *)(thread->tcb);

    while (tcb->thread_status != EXITED) {
        thread_yield();
    }

    if (retval != NULL) {
        *retval = tcb->exit_status;
    }

    free(tcb->stack_pointer);
    free(tcb);

    return 0;
}

void thread_exit(int status) {
    current_running->exit_status = status;
    current_running->thread_status = EXITED;

    // Call the scheduler to select the next thread
    scheduler_entry();
}

void scheduler() {
    printf("Before scheduling:\n");
    debug_print_current_running();

    if (is_empty(ready_queue)) {
        printf("No more threads to schedule.\n");
        exit_handler();
        return;
    }

    // Dequeue the next thread to run
    node_t *next_node = dequeue(&ready_queue);
    tcb_t *next_thread = (tcb_t *)(next_node->thread);
    free(next_node);
    // printf("Next thread:\n Thread ID: %d, Status: %s, CPU Time: %llu\n", next_thread->tid, status[next_thread->thread_status], (unsigned long long)next_thread->cpu_time);

    current_running = next_thread;

    // If the thread is running for the first time, call its start routine
    if (next_thread->thread_status == FIRST_TIME) {
        next_thread->thread_status = READY;
        next_thread->start_routine(next_thread->arg); //shit's happening here...
        printf("After scheduling:\n");
        debug_print_current_running();
        exit_handler();  // If the start routine returns, exit the thread
    }
}

void exit_handler() {
    printf("Thread %d has exited\n", current_running->tid);
    thread_exit(-1);
}
