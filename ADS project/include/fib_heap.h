/*
 * ============================================================================
 *  Emergency Response Dispatch & Optimal Routing System
 *  fib_heap.h — Fibonacci Heap (Priority Queue)
 * ============================================================================
 */

#ifndef FIB_HEAP_H
#define FIB_HEAP_H

#include "common.h"

/* ── Fibonacci Heap Node ──────────────────────────────────────────────── */

typedef struct FibNode {
    int    id;              /* vehicle ID or graph node ID               */
    double key;             /* priority key (travel time / distance)     */
    int    degree;          /* number of children                        */
    int    mark;            /* whether this node has lost a child        */

    struct FibNode *parent;
    struct FibNode *child;  /* pointer to one child (circular list)      */
    struct FibNode *left;   /* left sibling in circular doubly-linked    */
    struct FibNode *right;  /* right sibling in circular doubly-linked   */
} FibNode;

/* ── Fibonacci Heap ───────────────────────────────────────────────────── */

typedef struct {
    FibNode *min;           /* pointer to the minimum node               */
    int      count;         /* total number of nodes in the heap         */
} FibHeap;

/* ── Public API ───────────────────────────────────────────────────────── */

/* Create and return a new empty Fibonacci Heap */
FibHeap *fib_heap_create(void);

/* Insert a node with given ID and key. Returns pointer to the new node. */
FibNode *fib_heap_insert(FibHeap *h, int id, double key);

/* Return the minimum node without removing it. Returns NULL if empty. */
FibNode *fib_heap_peek_min(FibHeap *h);

/* Extract (remove and return) the minimum node. Caller must free it. */
FibNode *fib_heap_extract_min(FibHeap *h);

/* Decrease the key of a node. Returns 0 on success, -1 on error. */
int fib_heap_decrease_key(FibHeap *h, FibNode *node, double newKey);

/* Check if the heap is empty */
int fib_heap_is_empty(FibHeap *h);

/* Free the entire heap and all its nodes */
void fib_heap_free(FibHeap *h);

/* Print the heap contents (for debugging) */
void fib_heap_print(FibHeap *h);

#endif /* FIB_HEAP_H */
