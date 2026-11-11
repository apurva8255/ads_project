/*
 * ============================================================================
 *  Emergency Response Dispatch & Optimal Routing System
 *  fib_heap.c — Fibonacci Heap (Priority Queue) implementation
 * ============================================================================
 *
 *  A Fibonacci Heap is a collection of min-heap-ordered trees stored in a
 *  circular doubly-linked root list.  It provides:
 *
 *      Insert          O(1)
 *      Peek Min        O(1)
 *      Extract Min     O(log n) amortized
 *      Decrease Key    O(1) amortized
 *
 *  This makes it ideal for Dijkstra's shortest path algorithm.
 * ============================================================================
 */

#include "fib_heap.h"
#include <math.h>

/* ══════════════════════════════════════════════════════════════════════════
 *   Internal helpers
 * ══════════════════════════════════════════════════════════════════════════ */

/* ── Create a single isolated FibNode ─────────────────────────────────── */

static FibNode *create_node(int id, double key) {
    FibNode *n = (FibNode *)malloc(sizeof(FibNode));
    if (!n) {
        fprintf(stderr, "[FibHeap] Error: Memory allocation failed.\n");
        return NULL;
    }
    n->id     = id;
    n->key    = key;
    n->degree = 0;
    n->mark   = 0;
    n->parent = NULL;
    n->child  = NULL;
    n->left   = n;    /* points to itself (circular list of 1) */
    n->right  = n;
    return n;
}

/* ── Insert node `b` to the RIGHT of node `a` in a circular list ──────── */

static void list_insert(FibNode *a, FibNode *b) {
    b->right       = a->right;
    b->left        = a;
    a->right->left = b;
    a->right       = b;
}

/* ── Remove node `n` from its circular list ───────────────────────────── */

static void list_remove(FibNode *n) {
    n->left->right = n->right;
    n->right->left = n->left;
    n->left  = n;
    n->right = n;
}

/* ── Merge two circular lists (splice list of `b` into list of `a`) ──── */

static void list_merge(FibNode *a, FibNode *b) {
    if (!a || !b) return;

    FibNode *aRight = a->right;
    FibNode *bLeft  = b->left;

    a->right      = b;
    b->left       = a;
    bLeft->right  = aRight;
    aRight->left  = bLeft;
}

/* ── Make node `child` a child of node `parent` ───────────────────────── */

static void link(FibNode *child, FibNode *parent) {
    /* remove child from root list */
    list_remove(child);

    /* make child a child of parent */
    child->parent = parent;
    if (!parent->child) {
        parent->child = child;
        child->left   = child;
        child->right  = child;
    } else {
        list_insert(parent->child, child);
    }

    parent->degree++;
    child->mark = 0;
}

/* ── Consolidate: merge trees of equal degree ─────────────────────────── */

static void consolidate(FibHeap *h) {
    if (!h->min) return;

    /* max possible degree is O(log n) */
    int maxDegree = (int)(log((double)h->count) / log(1.618)) + 2;
    FibNode **degreeTable = (FibNode **)calloc((size_t)(maxDegree + 1),
                                               sizeof(FibNode *));
    if (!degreeTable) {
        fprintf(stderr, "[FibHeap] Error: Consolidate alloc failed.\n");
        return;
    }

    /* collect all root nodes into an array first (the circular list
       will be modified during linking, so we need a snapshot) */
    int rootCount = 0;
    FibNode *cur = h->min;
    do {
        rootCount++;
        cur = cur->right;
    } while (cur != h->min);

    FibNode **roots = (FibNode **)malloc(sizeof(FibNode *) * (size_t)rootCount);
    if (!roots) { free(degreeTable); return; }

    cur = h->min;
    for (int i = 0; i < rootCount; i++) {
        roots[i] = cur;
        cur = cur->right;
    }

    /* link trees of equal degree */
    for (int i = 0; i < rootCount; i++) {
        FibNode *x = roots[i];
        int d = x->degree;

        while (d <= maxDegree && degreeTable[d]) {
            FibNode *y = degreeTable[d];
            if (x->key > y->key) {
                /* swap so x always has the smaller key */
                FibNode *tmp = x; x = y; y = tmp;
            }
            /* make y a child of x */
            link(y, x);
            degreeTable[d] = NULL;
            d++;
        }
        if (d <= maxDegree)
            degreeTable[d] = x;
    }

    /* rebuild the root list and find the new min */
    h->min = NULL;
    for (int i = 0; i <= maxDegree; i++) {
        if (degreeTable[i]) {
            FibNode *n = degreeTable[i];
            n->left  = n;
            n->right = n;
            n->parent = NULL;

            if (!h->min) {
                h->min = n;
            } else {
                list_insert(h->min, n);
                if (n->key < h->min->key)
                    h->min = n;
            }
        }
    }

    free(degreeTable);
    free(roots);
}

/* ── Cut: remove node from its parent's child list, add to root list ──── */

static void cut(FibHeap *h, FibNode *node, FibNode *parent) {
    /* remove node from parent's child list */
    if (node->right == node) {
        /* only child */
        parent->child = NULL;
    } else {
        if (parent->child == node)
            parent->child = node->right;
        list_remove(node);
    }
    parent->degree--;

    /* add node to root list */
    node->parent = NULL;
    node->mark   = 0;
    node->left   = node;
    node->right  = node;
    list_insert(h->min, node);
}

/* ── Cascading cut ────────────────────────────────────────────────────── */

static void cascading_cut(FibHeap *h, FibNode *node) {
    FibNode *parent = node->parent;
    if (parent) {
        if (!node->mark) {
            node->mark = 1;
        } else {
            cut(h, node, parent);
            cascading_cut(h, parent);
        }
    }
}

/* ── Recursive free ───────────────────────────────────────────────────── */

static void free_nodes(FibNode *node) {
    if (!node) return;

    FibNode *cur = node;
    do {
        FibNode *next = cur->right;
        free_nodes(cur->child);
        free(cur);
        cur = next;
    } while (cur != node);
}

/* ── Recursive print helper ───────────────────────────────────────────── */

static void print_tree(FibNode *node, int depth) {
    if (!node) return;

    FibNode *cur = node;
    do {
        for (int i = 0; i < depth; i++) printf("  ");
        printf("ID: %d  Key: %.2f  Deg: %d  Mark: %d\n",
               cur->id, cur->key, cur->degree, cur->mark);
        print_tree(cur->child, depth + 1);
        cur = cur->right;
    } while (cur != node);
}

/* ══════════════════════════════════════════════════════════════════════════
 *   Public API
 * ══════════════════════════════════════════════════════════════════════════ */

FibHeap *fib_heap_create(void) {
    FibHeap *h = (FibHeap *)malloc(sizeof(FibHeap));
    if (!h) {
        fprintf(stderr, "[FibHeap] Error: Memory allocation failed.\n");
        return NULL;
    }
    h->min   = NULL;
    h->count = 0;
    return h;
}

FibNode *fib_heap_insert(FibHeap *h, int id, double key) {
    FibNode *n = create_node(id, key);
    if (!n) return NULL;

    if (!h->min) {
        h->min = n;
    } else {
        list_insert(h->min, n);
        if (n->key < h->min->key)
            h->min = n;
    }

    h->count++;
    return n;
}

FibNode *fib_heap_peek_min(FibHeap *h) {
    return h ? h->min : NULL;
}

FibNode *fib_heap_extract_min(FibHeap *h) {
    if (!h || !h->min) return NULL;

    FibNode *z = h->min;

    /* move all children of z into the root list */
    if (z->child) {
        FibNode *child = z->child;
        /* clear parent pointers */
        FibNode *cur = child;
        do {
            cur->parent = NULL;
            cur = cur->right;
        } while (cur != child);

        /* merge child list into root list */
        list_merge(z, child);
        z->child = NULL;
    }

    /* remove z from root list */
    if (z->right == z) {
        /* z was the only root */
        h->min = NULL;
    } else {
        h->min = z->right;
        list_remove(z);
        consolidate(h);
    }

    h->count--;
    z->left  = z;
    z->right = z;
    return z;
}

int fib_heap_decrease_key(FibHeap *h, FibNode *node, double newKey) {
    if (!h || !node) return -1;
    if (newKey > node->key) {
        fprintf(stderr, "[FibHeap] Error: New key %.2f is greater than "
                "current key %.2f.\n", newKey, node->key);
        return -1;
    }

    node->key = newKey;
    FibNode *parent = node->parent;

    if (parent && node->key < parent->key) {
        cut(h, node, parent);
        cascading_cut(h, parent);
    }

    if (node->key < h->min->key)
        h->min = node;

    return 0;
}

int fib_heap_is_empty(FibHeap *h) {
    return (!h || h->count == 0);
}

void fib_heap_free(FibHeap *h) {
    if (!h) return;
    if (h->min) {
        free_nodes(h->min);
    }
    free(h);
}

void fib_heap_print(FibHeap *h) {
    if (!h || !h->min) {
        printf("[FibHeap] (empty)\n");
        return;
    }
    printf("\n===== Fibonacci Heap (%d nodes, min key=%.2f) =====\n",
           h->count, h->min->key);
    print_tree(h->min, 1);
    printf("===================================================\n");
}
