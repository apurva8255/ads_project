/*
 * ============================================================================
 *  Emergency Response Dispatch & Optimal Routing System
 *  queue.c — Emergency Call Priority Queue Implementation
 * ============================================================================
 */

#include "queue.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ── Initialisation ──────────────────────────────────────────────────── */

void queue_init(EmergencyQueue *q) {
    if (!q) return;
    q->head = NULL;
    q->count = 0;
    q->nextCallID = 1;
}

/* ── Enqueue (Priority Ordered) ──────────────────────────────────────── */

int queue_enqueue(EmergencyQueue *q, EmergencyType type, 
                  const char *locationName, int priority, int timestamp) {
    if (!q) return -1;
    if (q->count >= MAX_QUEUE_SIZE) {
        fprintf(stderr, "[Queue] Error: Queue is at maximum capacity (%d)!\n", MAX_QUEUE_SIZE);
        return -1;
    }

    QueueNode *newNode = (QueueNode *)malloc(sizeof(QueueNode));
    if (!newNode) {
        fprintf(stderr, "[Queue] Error: Memory allocation failed.\n");
        return -1;
    }

    /* Populate the Call struct */
    newNode->call.callID = q->nextCallID++;
    newNode->call.type = type;
    strncpy(newNode->call.locationName, locationName, MAX_NAME_LEN - 1);
    newNode->call.locationName[MAX_NAME_LEN - 1] = '\0';
    newNode->call.locationNodeID = -1; /* To be resolved by Phase 9 Trie */
    newNode->call.priority = priority;
    newNode->call.timestamp = timestamp;
    newNode->next = NULL;

    /* Priority Queue Insertion Logic:
       Insert before the first node that has a *higher numeric* priority value 
       (since lower number = higher urgency). 
       If identical priority, insert after existing ones to maintain FIFO tie-breaks. */
       
    if (q->head == NULL || priority < q->head->call.priority) {
        /* New node becomes the head */
        newNode->next = q->head;
        q->head = newNode;
    } else {
        /* Traverse to find the correct insertion spot */
        QueueNode *current = q->head;
        while (current->next != NULL && current->next->call.priority <= priority) {
            current = current->next;
        }
        newNode->next = current->next;
        current->next = newNode;
    }

    q->count++;
    return 0;
}

/* ── Dequeue ─────────────────────────────────────────────────────────── */

int queue_dequeue(EmergencyQueue *q, EmergencyCall *outCall) {
    if (!q || q->head == NULL) {
        return 0;
    }

    QueueNode *temp = q->head;
    *outCall = temp->call;
    
    q->head = q->head->next;
    free(temp);
    q->count--;
    
    return 1;
}

/* ── Utilities ───────────────────────────────────────────────────────── */

int queue_is_empty(const EmergencyQueue *q) {
    return (!q || q->count == 0);
}

void queue_display(const EmergencyQueue *q) {
    if (!q) return;
    
    printf("\n===== Pending EmergenciesQueue: %d calls =====\n", q->count);
    QueueNode *curr = q->head;
    while (curr) {
        printf("  [Call %2d] Priority %d - %-10s at '%s' (T=%d)\n", 
               curr->call.callID, 
               curr->call.priority,
               emergency_type_str(curr->call.type),
               curr->call.locationName,
               curr->call.timestamp);
        curr = curr->next;
    }
    printf("==============================================\n");
}

void queue_free(EmergencyQueue *q) {
    if (!q) return;
    QueueNode *curr = q->head;
    while (curr) {
        QueueNode *next = curr->next;
        free(curr);
        curr = next;
    }
    q->head = NULL;
    q->count = 0;
}
