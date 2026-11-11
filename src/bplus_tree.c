/*
 * ============================================================================
 *  Emergency Response Dispatch & Optimal Routing System
 *  bplus_tree.c — B+ Tree Facility Database Implementation
 * ============================================================================
 */

#include "bplus_tree.h"

/* ── Internal Structures ─────────────────────────────────────────────── */

typedef struct {
    int         promoted_key;
    BPTreeNode *new_sibling;
} InsertResult;

/* ── Node Creation ───────────────────────────────────────────────────── */

static BPTreeNode *bptree_node_create(int isLeaf) {
    BPTreeNode *node = (BPTreeNode *)malloc(sizeof(BPTreeNode));
    if (!node) {
        fprintf(stderr, "[B+Tree] Memory allocation failed.\n");
        return NULL;
    }
    node->isLeaf = isLeaf;
    node->numKeys = 0;
    node->next = NULL;
    for (int i = 0; i < BPTREE_ORDER; i++) {
        node->children[i] = NULL;
    }
    return node;
}

/* ── Initialisation ──────────────────────────────────────────────────── */

void bptree_init(BPTree *tree) {
    tree->root = bptree_node_create(1);
    tree->count = 0;
}

/* ── Recursive Insertion Logic ───────────────────────────────────────── */

static InsertResult insert_recursive(BPTreeNode *node, Facility f) {
    InsertResult res = {-1, NULL};

    if (node->isLeaf) {
        int pos = 0;
        while (pos < node->numKeys && node->keys[pos] < f.id) pos++;

        if (pos < node->numKeys && node->keys[pos] == f.id) {
            /* Duplicate */
            return res;
        }

        Facility temp_facs[BPTREE_ORDER];
        int temp_keys[BPTREE_ORDER];
        
        for (int i = 0; i < pos; i++) {
            temp_keys[i] = node->keys[i];
            temp_facs[i] = node->facilities[i];
        }
        temp_keys[pos] = f.id;
        temp_facs[pos] = f;
        for (int i = pos; i < node->numKeys; i++) {
            temp_keys[i + 1] = node->keys[i];
            temp_facs[i + 1] = node->facilities[i];
        }
        
        int total = node->numKeys + 1;
        
        if (total < BPTREE_ORDER) {
            node->numKeys = total;
            for (int i = 0; i < total; i++) {
                node->keys[i] = temp_keys[i];
                node->facilities[i] = temp_facs[i];
            }
        } else {
            /* Split Leaf */
            int split = total / 2;
            node->numKeys = split;
            for (int i = 0; i < split; i++) {
                node->keys[i] = temp_keys[i];
                node->facilities[i] = temp_facs[i];
            }
            
            BPTreeNode *new_leaf = bptree_node_create(1);
            new_leaf->numKeys = total - split;
            for (int i = 0; i < new_leaf->numKeys; i++) {
                new_leaf->keys[i] = temp_keys[split + i];
                new_leaf->facilities[i] = temp_facs[split + i];
            }
            new_leaf->next = node->next;
            node->next = new_leaf;
            
            res.promoted_key = new_leaf->keys[0];
            res.new_sibling = new_leaf;
        }
    } else {
        int pos = 0;
        while (pos < node->numKeys && f.id >= node->keys[pos]) pos++;
        
        InsertResult sub_res = insert_recursive(node->children[pos], f);
        if (sub_res.new_sibling) {
            int temp_keys[BPTREE_ORDER];
            BPTreeNode *temp_children[BPTREE_ORDER + 1];
            
            for (int i = 0; i < pos; i++) {
                temp_keys[i] = node->keys[i];
                temp_children[i] = node->children[i];
            }
            temp_keys[pos] = sub_res.promoted_key;
            temp_children[pos] = node->children[pos];
            temp_children[pos + 1] = sub_res.new_sibling;
            for (int i = pos; i < node->numKeys; i++) {
                temp_keys[i + 1] = node->keys[i];
                temp_children[i + 2] = node->children[i + 1];
            }
            
            int total = node->numKeys + 1;
            
            if (total < BPTREE_ORDER) {
                node->numKeys = total;
                for (int i = 0; i < total; i++) node->keys[i] = temp_keys[i];
                for (int i = 0; i <= total; i++) node->children[i] = temp_children[i];
            } else {
                /* Split Internal */
                int split = total / 2;
                node->numKeys = split;
                for (int i = 0; i < split; i++) node->keys[i] = temp_keys[i];
                for (int i = 0; i <= split; i++) node->children[i] = temp_children[i];
                
                res.promoted_key = temp_keys[split];
                
                BPTreeNode *new_internal = bptree_node_create(0);
                new_internal->numKeys = total - split - 1;
                for (int i = 0; i < new_internal->numKeys; i++) {
                    new_internal->keys[i] = temp_keys[split + 1 + i];
                }
                for (int i = 0; i <= new_internal->numKeys; i++) {
                    new_internal->children[i] = temp_children[split + 1 + i];
                }
                
                res.new_sibling = new_internal;
            }
        }
    }
    return res;
}

int bptree_insert(BPTree *tree, Facility f) {
    InsertResult res = insert_recursive(tree->root, f);
    if (res.new_sibling) {
        BPTreeNode *new_root = bptree_node_create(0);
        new_root->numKeys = 1;
        new_root->keys[0] = res.promoted_key;
        new_root->children[0] = tree->root;
        new_root->children[1] = res.new_sibling;
        tree->root = new_root;
    }
    tree->count++;
    return 0;
}

/* ── Search ──────────────────────────────────────────────────────────── */

static BPTreeNode *bptree_find_leaf(BPTreeNode *node, int facilityID) {
    if (!node) return NULL;
    if (node->isLeaf) return node;
    
    int pos = 0;
    while (pos < node->numKeys && facilityID >= node->keys[pos]) pos++;
    
    return bptree_find_leaf(node->children[pos], facilityID);
}

Facility *bptree_search(BPTree *tree, int facilityID) {
    BPTreeNode *leaf = bptree_find_leaf(tree->root, facilityID);
    if (!leaf) return NULL;
    
    for (int i = 0; i < leaf->numKeys; i++) {
        if (leaf->keys[i] == facilityID) {
            return &leaf->facilities[i];
        }
    }
    return NULL;
}

/* ── Queries ─────────────────────────────────────────────────────────── */

int bptree_range_query(BPTree *tree, int minID, int maxID, 
                       Facility results[], int maxResults) {
    BPTreeNode *leaf = bptree_find_leaf(tree->root, minID);
    int found = 0;
    
    while (leaf && found < maxResults) {
        for (int i = 0; i < leaf->numKeys && found < maxResults; i++) {
            int key = leaf->keys[i];
            if (key >= minID && key <= maxID) {
                results[found++] = leaf->facilities[i];
            }
            if (key > maxID) return found; /* Stop early */
        }
        leaf = leaf->next;
    }
    return found;
}

int bptree_get_by_type(BPTree *tree, FacilityType type, 
                       Facility results[], int maxResults) {
    BPTreeNode *leaf = tree->root;
    while (leaf && !leaf->isLeaf) {
        leaf = leaf->children[0]; /* go leftmost */
    }
    
    int found = 0;
    while (leaf && found < maxResults) {
        for (int i = 0; i < leaf->numKeys && found < maxResults; i++) {
            if (leaf->facilities[i].type == type) {
                results[found++] = leaf->facilities[i];
            }
        }
        leaf = leaf->next;
    }
    return found;
}

/* ── Display ─────────────────────────────────────────────────────────── */

void bptree_display(const BPTree *tree) {
    printf("\n===== Facility Database (%d records) =====\n", tree->count);
    BPTreeNode *leaf = tree->root;
    while (leaf && !leaf->isLeaf) {
        leaf = leaf->children[0];
    }
    
    while (leaf) {
        for (int i = 0; i < leaf->numKeys; i++) {
            Facility f = leaf->facilities[i];
            printf("  [%2d] %-15s (Node %2d)  Type: %-15s  Capacity: %3d\n",
                   f.id, f.name, f.locationNodeID,
                   facility_type_str(f.type), f.capacity);
        }
        leaf = leaf->next;
    }
    printf("==========================================\n");
}

/* ── Free ────────────────────────────────────────────────────────────── */

static void free_node(BPTreeNode *node) {
    if (!node) return;
    if (!node->isLeaf) {
        for (int i = 0; i <= node->numKeys; i++) {
            free_node(node->children[i]);
        }
    }
    free(node);
}

void bptree_free(BPTree *tree) {
    free_node(tree->root);
    tree->root = NULL;
    tree->count = 0;
}

/* ── File Loading & QuadTree Synergy ─────────────────────────────────── */

int bptree_load_from_file(BPTree *tree, const char *filename, QuadTree *qt) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        fprintf(stderr, "[B+Tree] Cannot open file '%s'.\n", filename);
        return -1;
    }

    char token[MAX_NAME_LEN];
    int count;

    if (fscanf(fp, "%s %d", token, &count) != 2 || strcmp(token, "FACILITIES") != 0) {
        fprintf(stderr, "[B+Tree] Expected 'FACILITIES <count>' header.\n");
        fclose(fp);
        return -1;
    }

    for (int i = 0; i < count; i++) {
        Facility f;
        int type_val;
        if (fscanf(fp, "%d %d %d %lf %lf %d %63s",
                   &f.id, &type_val, &f.locationNodeID,
                   &f.position.x, &f.position.y,
                   &f.capacity, f.name) != 7) {
            fprintf(stderr, "[B+Tree] Error parsing facility on line %d.\n", i + 1);
            fclose(fp);
            return -1;
        }
        
        f.type = (FacilityType)type_val;
        
        /* 1) Insert into B+ Tree */
        bptree_insert(tree, f);
        
        /* 2) Insert spatial marker into Quad Tree */
        if (qt) {
            QTPoint p;
            p.x = f.position.x;
            p.y = f.position.y;
            p.entityType = QT_ENTITY_FACILITY;
            p.entityID = f.id;
            p.entitySubType = f.type;
            quadtree_insert(qt, p);
        }
    }

    fclose(fp);
    printf("[B+Tree] Loaded %d facilities and injected them into Spatial DB.\n", tree->count);
    return 0;
}
