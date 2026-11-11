/*
 * ============================================================================
 *  Emergency Response Dispatch & Optimal Routing System
 *  avl_tree.h — AVL Tree for Vehicle Database
 * ============================================================================
 */

#ifndef AVL_TREE_H
#define AVL_TREE_H

#include "common.h"

/* ── AVL Tree Node ────────────────────────────────────────────────────── */

typedef struct AVLNode {
    Vehicle         vehicle;    /* vehicle record stored in this node     */
    int             height;     /* height of this node in the tree        */
    struct AVLNode *left;
    struct AVLNode *right;
} AVLNode;

/* ── AVL Tree wrapper ─────────────────────────────────────────────────── */

typedef struct {
    AVLNode *root;
    int      count;   /* total number of vehicles stored */
} AVLTree;

/* ── Public API ───────────────────────────────────────────────────────── */

/* Initialise an empty AVL tree */
void avl_init(AVLTree *tree);

/* Insert a vehicle into the tree. Returns 0 on success, -1 on failure. */
int avl_insert(AVLTree *tree, Vehicle vehicle);

/* Delete a vehicle by ID. Returns 0 on success, -1 if not found. */
int avl_delete(AVLTree *tree, int vehicleID);

/* Search for a vehicle by ID. Returns pointer to Vehicle or NULL. */
Vehicle *avl_search(AVLTree *tree, int vehicleID);

/* Update the status of a vehicle. Returns 0 on success, -1 if not found. */
int avl_update_status(AVLTree *tree, int vehicleID, VehicleStatus newStatus);

/* Update the location of a vehicle. Returns 0 on success, -1 if not found. */
int avl_update_location(AVLTree *tree, int vehicleID, int newNodeID,
                        double x, double y);

/*
 * Get all available vehicles of a given type.
 * Fills `results` array (up to `maxResults` entries).
 * Returns the number of matching vehicles found.
 */
int avl_get_available(AVLTree *tree, VehicleType type,
                      Vehicle results[], int maxResults);

/* Print all vehicles in-order (sorted by ID) */
void avl_inorder(const AVLTree *tree);

/* Free all nodes in the tree */
void avl_free(AVLTree *tree);

/* Load vehicles from a file. Returns number loaded, or -1 on error.
 *
 * File format:
 *   VEHICLES <count>
 *   <id> <type: 0=Ambulance 1=Police 2=Fire> <locationNodeID> <x> <y> <name>
 *   ...
 */
int avl_load_from_file(AVLTree *tree, const char *filename);

#endif /* AVL_TREE_H */
