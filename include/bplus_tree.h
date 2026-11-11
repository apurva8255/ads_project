/*
 * ============================================================================
 *  Emergency Response Dispatch & Optimal Routing System
 *  bplus_tree.h — B+ Tree for Facility Database
 * ============================================================================
 */

#ifndef BPLUS_TREE_H
#define BPLUS_TREE_H

#include "common.h"
#include "quadtree.h"

/* ── B+ Tree Node ─────────────────────────────────────────────────────── */

typedef struct BPTreeNode {
    int                 isLeaf;                  /* 1 if leaf node, 0 otherwise */
    int                 numKeys;                 /* number of active keys       */
    int                 keys[BPTREE_ORDER - 1];  /* array of keys               */
    
    /* Internal node pointers */
    struct BPTreeNode  *children[BPTREE_ORDER];
    
    /* Leaf node specific data */
    Facility            facilities[BPTREE_ORDER - 1];
    struct BPTreeNode  *next;                    /* pointer to next leaf        */
} BPTreeNode;

/* ── B+ Tree wrapper ──────────────────────────────────────────────────── */

typedef struct {
    BPTreeNode *root;
    int         count;     /* total number of facilities stored */
} BPTree;

/* ── Public API ───────────────────────────────────────────────────────── */

/* Initialise an empty B+ Tree */
void bptree_init(BPTree *tree);

/* Insert a facility record into the B+ tree. Returns 0 on success. */
int bptree_insert(BPTree *tree, Facility f);

/* Search for a facility by its ID. Returns a pointer or NULL if not found. */
Facility *bptree_search(BPTree *tree, int facilityID);

/* 
 * Find all facilities within an ID range [minID, maxID].
 * Fills `results` array up to `maxResults`.
 * Returns the number of facilities found.
 */
int bptree_range_query(BPTree *tree, int minID, int maxID, 
                       Facility results[], int maxResults);

/* 
 * Retrieve all facilities of a given type via full leaf scan.
 * Fills `results` array up to `maxResults`.
 * Returns the number of facilities found.
 */
int bptree_get_by_type(BPTree *tree, FacilityType type, 
                       Facility results[], int maxResults);

/* Print all facilities in the database via leaf-level linked list traversal. */
void bptree_display(const BPTree *tree);

/* Free the entire B+ Tree footprint */
void bptree_free(BPTree *tree);

/* 
 * Load facility data from a file, inserting it into the B+ tree 
 * and synchronously injecting it as QtPoints into the Spatial QuadTree.
 * Returns 0 on success, -1 on failure.
 */
int bptree_load_from_file(BPTree *tree, const char *filename, QuadTree *qt);

#endif /* BPLUS_TREE_H */
