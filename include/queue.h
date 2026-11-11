/*
 * ============================================================================
 *  Emergency Response Dispatch & Optimal Routing System
 *  queue.h — Emergency Call Priority Queue
 * ============================================================================
 */

#ifndef QUEUE_H
#define QUEUE_H

#include "common.h"

/* ── Queue Node ──────────────────────────────────────────────────────── */

typedef struct QueueNode {
    EmergencyCall     call;
    struct QueueNode *next;
} QueueNode;

/* ── Priority Queue wrapper ──────────────────────────────────────────── */

typedef struct {
    QueueNode *head;
    int        count;
    int        nextCallID;
} EmergencyQueue;

/* ── Public API ──────────────────────────────────────────────────────── */

/* Initialise an empty priority queue */
void queue_init(EmergencyQueue *q);

/* 
 * Insert an emergency call into the queue based on its priority.
 * Lowest priority value (e.g. 1) means highest urgency.
 * Enqueue inserts before the first node found that has a strictly lower urgency.
 * Returns 0 on success, -1 if queue size exceeds MAX_QUEUE_SIZE.
 */
int queue_enqueue(EmergencyQueue *q, EmergencyType type, 
                  const char *locationName, int priority, int timestamp);

/* 
 * Remove and return the highest priority call (the head).
 * Returns 1 if a call was retrieved, 0 if the queue was empty.
 */
int queue_dequeue(EmergencyQueue *q, EmergencyCall *outCall);

/* Check if the queue is empty. Returns 1 if empty, 0 otherwise. */
int queue_is_empty(const EmergencyQueue *q);

/* Display all pending calls in the queue in their sorted order. */
void queue_display(const EmergencyQueue *q);

/* Free all nodes in the queue. */
void queue_free(EmergencyQueue *q);

#endif /* QUEUE_H */
