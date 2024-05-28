#include <stdint.h>
#include <thread.h>
#include <queue.h>
#include <stdio.h>

void queue_init(node_t **queue) {
  *queue = NULL;
}

node_t *dequeue(node_t **queue) {
  if (*queue == NULL) {
    return NULL;
  }
  node_t *front = *queue;
  *queue = front->next;
  front->next = NULL;
  return front;
}

void enqueue(node_t **queue, node_t *item) {
  char *status[] = {"FIRST_TIME", "READY", "BLOCKED", "EXITED"};

  printf("enqueue\n");
  
  tcb_t *tcb = (tcb_t *)(item->thread);
  printf("Thread ID: %d, Status: %s, CPU Time: %llu\n", tcb->tid, status[tcb->thread_status], (unsigned long long)tcb->cpu_time);
  
  item->next = NULL;
  if (*queue == NULL) {
    *queue = item;
  } else {
    node_t *temp = *queue;
    while (temp->next != NULL) {
      temp = temp->next;
    }
    temp->next = item;
  }
}

int is_empty(node_t *queue) {
  return queue == NULL;
}

node_t *peek(node_t *queue) {
  return queue;
}

typedef int (*node_lte)(node_t *a, node_t *b);

// Comparison function based on CPU time (execution time)
// int compare_by_cpu_time(node_t *a, node_t *b) {
//   uint64_t cpu_time_a = ((thread_t *)(a->thread))->tcb->cpu_time;
//   uint64_t cpu_time_b = ((thread_t *)(b->thread))->tcb->cpu_time;
  
//   // Compare CPU times
//   if (cpu_time_a <= cpu_time_b) {
//     return 1;  // a is less-than-or-equal-to b
//   } else {
//     return 0;  // a is greater than b
//   }
// }

void enqueue_sort(node_t **q, node_t *item, node_lte comp) {
  if (*q == NULL || comp(item, *q)) {
    item->next = *q;
    *q = item;
  } else {
    node_t *current = *q;
    while (current->next != NULL && comp(current->next, item)) {
      current = current->next;
    }
    item->next = current->next;
    current->next = item;
  }
}
