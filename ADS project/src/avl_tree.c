/*
 * ============================================================================
 *  Emergency Response Dispatch & Optimal Routing System
 *  avl_tree.c — AVL Tree for Vehicle Database (implementation)
 * ============================================================================
 */

#include "avl_tree.h"

/* ══════════════════════════════════════════════════════════════════════════
 *   Internal helpers
 * ══════════════════════════════════════════════════════════════════════════ */

/* ── Height of a node (NULL-safe) ─────────────────────────────────────── */

static int height(const AVLNode *n) {
    return n ? n->height : 0;
}

/* ── Recalculate height from children ─────────────────────────────────── */

static int max_int(int a, int b) { return a > b ? a : b; }

static void update_height(AVLNode *n) {
    if (n) n->height = 1 + max_int(height(n->left), height(n->right));
}

/* ── Balance factor ───────────────────────────────────────────────────── */

static int balance_factor(const AVLNode *n) {
    return n ? height(n->left) - height(n->right) : 0;
}

/* ── Rotations ────────────────────────────────────────────────────────── */

/*
 *     y            x
 *    / \          / \
 *   x   C  -->  A   y
 *  / \              / \
 * A   B            B   C
 */
static AVLNode *rotate_right(AVLNode *y) {
    AVLNode *x  = y->left;
    AVLNode *B  = x->right;

    x->right = y;
    y->left  = B;

    update_height(y);
    update_height(x);
    return x;
}

/*
 *   x              y
 *  / \            / \
 * A   y    -->   x   C
 *    / \        / \
 *   B   C      A   B
 */
static AVLNode *rotate_left(AVLNode *x) {
    AVLNode *y  = x->right;
    AVLNode *B  = y->left;

    y->left  = x;
    x->right = B;

    update_height(x);
    update_height(y);
    return y;
}

/* ── Rebalance a node after insert / delete ───────────────────────────── */

static AVLNode *rebalance(AVLNode *n) {
    update_height(n);
    int bf = balance_factor(n);

    /* Left-heavy */
    if (bf > 1) {
        if (balance_factor(n->left) < 0)       /* Left-Right case */
            n->left = rotate_left(n->left);
        return rotate_right(n);                 /* Left-Left case  */
    }

    /* Right-heavy */
    if (bf < -1) {
        if (balance_factor(n->right) > 0)       /* Right-Left case */
            n->right = rotate_right(n->right);
        return rotate_left(n);                  /* Right-Right case */
    }

    return n;   /* already balanced */
}

/* ── Create a new AVL node ────────────────────────────────────────────── */

static AVLNode *create_node(Vehicle vehicle) {
    AVLNode *n = (AVLNode *)malloc(sizeof(AVLNode));
    if (!n) {
        fprintf(stderr, "[AVL] Error: Memory allocation failed.\n");
        return NULL;
    }
    n->vehicle = vehicle;
    n->height  = 1;
    n->left    = NULL;
    n->right   = NULL;
    return n;
}

/* ── Recursive insert ─────────────────────────────────────────────────── */

static AVLNode *insert_node(AVLNode *node, Vehicle vehicle, int *success) {
    if (!node) {
        AVLNode *n = create_node(vehicle);
        if (n) *success = 1;
        return n;
    }

    if (vehicle.id < node->vehicle.id) {
        node->left = insert_node(node->left, vehicle, success);
    } else if (vehicle.id > node->vehicle.id) {
        node->right = insert_node(node->right, vehicle, success);
    } else {
        /* Duplicate ID — do not insert */
        fprintf(stderr, "[AVL] Warning: Vehicle ID %d already exists.\n",
                vehicle.id);
        *success = 0;
        return node;
    }

    return rebalance(node);
}

/* ── Find the in-order successor (leftmost node in right subtree) ────── */

static AVLNode *find_min(AVLNode *node) {
    while (node && node->left)
        node = node->left;
    return node;
}

/* ── Recursive delete ─────────────────────────────────────────────────── */

static AVLNode *delete_node(AVLNode *node, int vehicleID, int *success) {
    if (!node) {
        *success = 0;
        return NULL;
    }

    if (vehicleID < node->vehicle.id) {
        node->left = delete_node(node->left, vehicleID, success);
    } else if (vehicleID > node->vehicle.id) {
        node->right = delete_node(node->right, vehicleID, success);
    } else {
        /* Found the node to delete */
        *success = 1;

        if (!node->left || !node->right) {
            /* 0 or 1 child */
            AVLNode *child = node->left ? node->left : node->right;
            free(node);
            return child;   /* may be NULL */
        }

        /* 2 children: replace with in-order successor */
        AVLNode *successor = find_min(node->right);
        node->vehicle = successor->vehicle;
        node->right = delete_node(node->right, successor->vehicle.id, success);
    }

    return rebalance(node);
}

/* ── Recursive search ─────────────────────────────────────────────────── */

static AVLNode *search_node(AVLNode *node, int vehicleID) {
    if (!node) return NULL;

    if (vehicleID < node->vehicle.id)
        return search_node(node->left, vehicleID);
    else if (vehicleID > node->vehicle.id)
        return search_node(node->right, vehicleID);
    else
        return node;
}

/* ── Recursive in-order collection of available vehicles of a type ────── */

static void collect_available(AVLNode *node, VehicleType type,
                              Vehicle results[], int maxResults, int *count) {
    if (!node || *count >= maxResults) return;

    collect_available(node->left, type, results, maxResults, count);

    if (node->vehicle.type == type &&
        node->vehicle.status == STATUS_AVAILABLE &&
        *count < maxResults) {
        results[(*count)++] = node->vehicle;
    }

    collect_available(node->right, type, results, maxResults, count);
}

/* ── Recursive in-order print ─────────────────────────────────────────── */

static void inorder_print(const AVLNode *node) {
    if (!node) return;

    inorder_print(node->left);

    printf("  [%3d] %-12s | %-10s | Location Node: %2d (%.1f, %.1f) | %s\n",
           node->vehicle.id,
           node->vehicle.name,
           vehicle_type_str(node->vehicle.type),
           node->vehicle.locationNodeID,
           node->vehicle.position.x,
           node->vehicle.position.y,
           vehicle_status_str(node->vehicle.status));

    inorder_print(node->right);
}

/* ── Recursive free ───────────────────────────────────────────────────── */

static void free_nodes(AVLNode *node) {
    if (!node) return;
    free_nodes(node->left);
    free_nodes(node->right);
    free(node);
}

/* ══════════════════════════════════════════════════════════════════════════
 *   Public API
 * ══════════════════════════════════════════════════════════════════════════ */

void avl_init(AVLTree *tree) {
    tree->root  = NULL;
    tree->count = 0;
}

int avl_insert(AVLTree *tree, Vehicle vehicle) {
    int success = 0;
    tree->root = insert_node(tree->root, vehicle, &success);
    if (success) tree->count++;
    return success ? 0 : -1;
}

int avl_delete(AVLTree *tree, int vehicleID) {
    int success = 0;
    tree->root = delete_node(tree->root, vehicleID, &success);
    if (success) tree->count--;
    return success ? 0 : -1;
}

Vehicle *avl_search(AVLTree *tree, int vehicleID) {
    AVLNode *n = search_node(tree->root, vehicleID);
    return n ? &n->vehicle : NULL;
}

int avl_update_status(AVLTree *tree, int vehicleID, VehicleStatus newStatus) {
    AVLNode *n = search_node(tree->root, vehicleID);
    if (!n) {
        fprintf(stderr, "[AVL] Vehicle %d not found.\n", vehicleID);
        return -1;
    }
    n->vehicle.status = newStatus;
    return 0;
}

int avl_update_location(AVLTree *tree, int vehicleID, int newNodeID,
                        double x, double y) {
    AVLNode *n = search_node(tree->root, vehicleID);
    if (!n) {
        fprintf(stderr, "[AVL] Vehicle %d not found.\n", vehicleID);
        return -1;
    }
    n->vehicle.locationNodeID = newNodeID;
    n->vehicle.position.x     = x;
    n->vehicle.position.y     = y;
    return 0;
}

int avl_get_available(AVLTree *tree, VehicleType type,
                      Vehicle results[], int maxResults) {
    int count = 0;
    collect_available(tree->root, type, results, maxResults, &count);
    return count;
}

void avl_inorder(const AVLTree *tree) {
    printf("\n===== Vehicle Database (%d vehicles) =====\n", tree->count);
    printf("  %-5s %-12s | %-10s | %-30s | %s\n",
           "ID", "Name", "Type", "Location", "Status");
    printf("  ----------------------------------------------------------"
           "-----------------\n");
    if (tree->root) {
        inorder_print(tree->root);
    } else {
        printf("  (empty)\n");
    }
    printf("====================================================\n");
}

void avl_free(AVLTree *tree) {
    free_nodes(tree->root);
    tree->root  = NULL;
    tree->count = 0;
}

int avl_load_from_file(AVLTree *tree, const char *filename) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        fprintf(stderr, "[AVL] Error: Cannot open file '%s'.\n", filename);
        return -1;
    }

    char token[MAX_NAME_LEN];
    int count;

    if (fscanf(fp, "%s %d", token, &count) != 2 ||
        strcmp(token, "VEHICLES") != 0) {
        fprintf(stderr, "[AVL] Error: Expected 'VEHICLES <count>' header.\n");
        fclose(fp);
        return -1;
    }

    int loaded = 0;
    for (int i = 0; i < count; i++) {
        Vehicle v;
        int typeInt;

        if (fscanf(fp, "%d %d %d %lf %lf %s",
                   &v.id, &typeInt, &v.locationNodeID,
                   &v.position.x, &v.position.y, v.name) != 6) {
            fprintf(stderr, "[AVL] Error: Bad vehicle entry at line %d.\n", i);
            fclose(fp);
            return -1;
        }

        v.type   = (VehicleType)typeInt;
        v.status = STATUS_AVAILABLE;   /* all vehicles start available */

        if (avl_insert(tree, v) == 0)
            loaded++;
    }

    fclose(fp);
    printf("[AVL] Loaded %d vehicles from '%s'.\n", loaded, filename);
    return loaded;
}
