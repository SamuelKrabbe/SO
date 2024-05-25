#ifndef QUEUE_H
#define QUEUE_H

#include <thread.h>
#include <stddef.h>

// Define the structure for a queue node
typedef struct node {
   tcb_t *head;
} node_t;

/* Initialize a queue. This sets the queue to an empty state. */
void queue_init(node_t **queue);

/* Remove and return the item at the front of the queue. Return NULL if the queue is empty. */
tcb_t *dequeue(node_t **queue);

/* Add an item to the back of the queue. */
void enqueue(node_t **queue, tcb_t *item);

/* Determine if the queue is empty.
 * Returns 1 if the queue is empty.
 * Returns 0 otherwise.
 */
int is_empty(node_t *queue);

/* Returns the first item in the queue.
 * Returns NULL if the queue is empty.
 */
tcb_t *peek(node_t *queue);

/* A comparison function should return:
 *  1 if a is less-than-or-equal-to b;
 *  0 otherwise.
 */
typedef int (*tcb_lte)(tcb_t *a, tcb_t *b);

/* Insert this item /item/ into the queue /q/
 * in ascending order with the less-than-or-equal-to
 * inequality /comp/.
 * If /q/ was sorted (w.r.t. /comp/) before the
 * call, then /q/ is sorted after the call.
 * This is the simple linear-time algorithm.
 */
void enqueue_sort(node_t **q, tcb_t *item, tcb_lte comp);

#endif /* QUEUE_H */
