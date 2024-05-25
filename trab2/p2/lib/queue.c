#include "queue.h"
#include <stdlib.h>

/* Initialize a queue. This sets the queue to an empty state. */
void queue_init(node_t **queue) {
  (*queue)->head = NULL;
}

/* Remove and return the item at the front of the queue. Return NULL if the queue is empty. */
tcb_t *dequeue(node_t **queue) {
  if ((*queue)->head == NULL) {
    return NULL; // Queue is empty
  }

  tcb_t *front = (*queue)->head;
  (*queue)->head = ((*queue)->head)->next;
  front->next = NULL; // Isolate the dequeued node
  return front;
}

/* Add an item to the back of the queue. */
void enqueue(node_t **queue, tcb_t *item) {
  item->next = NULL;
  if ((*queue)->head == NULL) {
    (*queue)->head = item;
  } else {
    tcb_t *current = (*queue)->head;
    while (current->next != NULL) {
      current = current->next;
    }
    current->next = item;
  }
}

/* Determine if the queue is empty.
 * Returns 1 if the queue is empty.
 * Returns 0 otherwise.
 */
int is_empty(node_t *queue) {
  return queue->head == NULL;
}

/* Returns the first item in the queue.
 * Returns NULL if the queue is empty.
 */
tcb_t *peek(node_t *queue) {
  return queue->head; // Returns NULL if the queue is empty
}

/* Insert this item /item/ into the queue /q/
 * in ascending order with the less-than-or-equal-to
 * inequality /comp/.
 * If /q/ was sorted (w.r.t. /comp/) before the
 * call, then /q/ is sorted after the call.
 * This is the simple linear-time algorithm.
 */
void enqueue_sort(node_t **q, tcb_t *item, tcb_lte comp) {
  if ((*q)->head == NULL || comp(item, (*q)->head)) {
    item->next = (*q)->head;
    (*q)->head = item;
  } else {
    tcb_t *current = (*q)->head;
    while (current->next != NULL && !comp(item, current->next)) {
      current = current->next;
    }
    item->next = current->next;
    current->next = item;
  }
}
