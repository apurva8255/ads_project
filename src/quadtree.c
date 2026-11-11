/*
 * ============================================================================
 *  Emergency Response Dispatch & Optimal Routing System
 *  quadtree.c — Quad Tree for Spatial City Partitioning (implementation)
 * ============================================================================
 */

#include "quadtree.h"
#include "avl_tree.h"
#include <math.h>

/* ── Boundary helpers ────────────────────────────────────────────────── */

QTBoundary qt_boundary_create(double cx, double cy,
                              double half_w, double half_h) {
    QTBoundary b;
    b.x_center    = cx;
    b.y_center    = cy;
    b.half_width  = half_w;
    b.half_height = half_h;
    return b;
}

int qt_boundary_contains(const QTBoundary *b, double x, double y) {
    return (x >= b->x_center - b->half_width  &&
            x <= b->x_center + b->half_width  &&
            y >= b->y_center - b->half_height &&
            y <= b->y_center + b->half_height);
}

int qt_boundary_intersects(const QTBoundary *a, const QTBoundary *b) {
    /* Two AABBs do NOT intersect iff they are separated on either axis */
    if (a->x_center + a->half_width  < b->x_center - b->half_width)  return 0;
    if (a->x_center - a->half_width  > b->x_center + b->half_width)  return 0;
    if (a->y_center + a->half_height < b->y_center - b->half_height) return 0;
    if (a->y_center - a->half_height > b->y_center + b->half_height) return 0;
    return 1;
}

/* ── Internal: create a new QTNode ───────────────────────────────────── */

static QTNode *qt_node_create(QTBoundary boundary) {
    QTNode *node = (QTNode *)malloc(sizeof(QTNode));
    if (!node) {
        fprintf(stderr, "[QuadTree] Error: Memory allocation failed.\n");
        return NULL;
    }
    node->boundary   = boundary;
    node->pointCount = 0;
    node->divided    = 0;
    node->nw = node->ne = node->sw = node->se = NULL;
    return node;
}

/* ── Internal: subdivide a node into four children ───────────────────── */

static void qt_subdivide(QTNode *node) {
    double cx = node->boundary.x_center;
    double cy = node->boundary.y_center;
    double hw = node->boundary.half_width  / 2.0;
    double hh = node->boundary.half_height / 2.0;

    node->nw = qt_node_create(qt_boundary_create(cx - hw, cy + hh, hw, hh));
    node->ne = qt_node_create(qt_boundary_create(cx + hw, cy + hh, hw, hh));
    node->sw = qt_node_create(qt_boundary_create(cx - hw, cy - hh, hw, hh));
    node->se = qt_node_create(qt_boundary_create(cx + hw, cy - hh, hw, hh));

    node->divided = 1;
}

/* ── Internal: recursive insert into a node ──────────────────────────── */

static int qt_node_insert(QTNode *node, QTPoint point) {
    /* Point must lie within this node's boundary */
    if (!qt_boundary_contains(&node->boundary, point.x, point.y)) {
        return 0;   /* not in this region */
    }

    /* If there is room, store it here */
    if (node->pointCount < QUADTREE_CAPACITY) {
        node->points[node->pointCount] = point;
        node->pointCount++;
        return 1;
    }

    /* Otherwise, subdivide if not already done */
    if (!node->divided) {
        qt_subdivide(node);
    }

    /* Try to insert into one of the four children */
    if (qt_node_insert(node->nw, point)) return 1;
    if (qt_node_insert(node->ne, point)) return 1;
    if (qt_node_insert(node->sw, point)) return 1;
    if (qt_node_insert(node->se, point)) return 1;

    /* Should not reach here if boundary logic is correct */
    return 0;
}

/* ── Internal: recursive range query ─────────────────────────────────── */

static int qt_node_query_range(const QTNode *node, const QTBoundary *range,
                               QTPoint results[], int maxResults, int found) {
    if (!node) return found;

    /* If this node's region does not intersect the query range, skip */
    if (!qt_boundary_intersects(&node->boundary, range)) {
        return found;
    }

    /* Check each point stored at this node */
    for (int i = 0; i < node->pointCount && found < maxResults; i++) {
        if (qt_boundary_contains(range, node->points[i].x,
                                         node->points[i].y)) {
            results[found] = node->points[i];
            found++;
        }
    }

    /* If node has children, recurse into them */
    if (node->divided) {
        found = qt_node_query_range(node->nw, range, results, maxResults, found);
        found = qt_node_query_range(node->ne, range, results, maxResults, found);
        found = qt_node_query_range(node->sw, range, results, maxResults, found);
        found = qt_node_query_range(node->se, range, results, maxResults, found);
    }

    return found;
}

/* ── Internal: squared distance helper ───────────────────────────────── */

static double sq_dist(double x1, double y1, double x2, double y2) {
    double dx = x1 - x2;
    double dy = y1 - y2;
    return dx * dx + dy * dy;
}

/* ── Internal: recursive nearest-point search ────────────────────────── */

static void qt_node_query_nearest(const QTNode *node, double qx, double qy,
                                  QTPoint *best, double *bestDistSq) {
    if (!node) return;

    /* Quick rejection: if the closest point of this boundary is farther
       than our current best, don't bother searching this subtree.       */
    double closest_x = qx;
    double closest_y = qy;

    double left   = node->boundary.x_center - node->boundary.half_width;
    double right  = node->boundary.x_center + node->boundary.half_width;
    double bottom = node->boundary.y_center - node->boundary.half_height;
    double top    = node->boundary.y_center + node->boundary.half_height;

    if (closest_x < left)   closest_x = left;
    if (closest_x > right)  closest_x = right;
    if (closest_y < bottom) closest_y = bottom;
    if (closest_y > top)    closest_y = top;

    if (sq_dist(qx, qy, closest_x, closest_y) > *bestDistSq) {
        return;   /* this entire region is farther than current best */
    }

    /* Check each point stored at this node */
    for (int i = 0; i < node->pointCount; i++) {
        double d = sq_dist(qx, qy, node->points[i].x, node->points[i].y);
        if (d < *bestDistSq) {
            *bestDistSq = d;
            *best = node->points[i];
        }
    }

    /* Recurse into children */
    if (node->divided) {
        qt_node_query_nearest(node->nw, qx, qy, best, bestDistSq);
        qt_node_query_nearest(node->ne, qx, qy, best, bestDistSq);
        qt_node_query_nearest(node->sw, qx, qy, best, bestDistSq);
        qt_node_query_nearest(node->se, qx, qy, best, bestDistSq);
    }
}

/* ── Internal: recursive nearest-point search by type ───────────────────────── */

static void qt_node_query_nearest_by_type(const QTNode *node, double qx, double qy,
                                          QTEntityType type, int subType,
                                          QTPoint *best, double *bestDistSq) {
    if (!node) return;

    double closest_x = qx;
    double closest_y = qy;

    double left   = node->boundary.x_center - node->boundary.half_width;
    double right  = node->boundary.x_center + node->boundary.half_width;
    double bottom = node->boundary.y_center - node->boundary.half_height;
    double top    = node->boundary.y_center + node->boundary.half_height;

    if (closest_x < left)   closest_x = left;
    if (closest_x > right)  closest_x = right;
    if (closest_y < bottom) closest_y = bottom;
    if (closest_y > top)    closest_y = top;

    if (sq_dist(qx, qy, closest_x, closest_y) > *bestDistSq) {
        return;
    }

    /* Check matching points stored at this node */
    for (int i = 0; i < node->pointCount; i++) {
        if (node->points[i].entityType == type && node->points[i].entitySubType == subType) {
            double d = sq_dist(qx, qy, node->points[i].x, node->points[i].y);
            if (d < *bestDistSq) {
                *bestDistSq = d;
                *best = node->points[i];
            }
        }
    }

    if (node->divided) {
        qt_node_query_nearest_by_type(node->nw, qx, qy, type, subType, best, bestDistSq);
        qt_node_query_nearest_by_type(node->ne, qx, qy, type, subType, best, bestDistSq);
        qt_node_query_nearest_by_type(node->sw, qx, qy, type, subType, best, bestDistSq);
        qt_node_query_nearest_by_type(node->se, qx, qy, type, subType, best, bestDistSq);
    }
}

/* ── Internal: recursive remove ──────────────────────────────────────── */

static int qt_node_remove(QTNode *node, QTEntityType type, int entityID) {
    if (!node) return 0;

    /* Search points at this node */
    for (int i = 0; i < node->pointCount; i++) {
        if (node->points[i].entityType == type &&
            node->points[i].entityID   == entityID) {
            /* Shift remaining points down to fill the gap */
            for (int j = i; j < node->pointCount - 1; j++) {
                node->points[j] = node->points[j + 1];
            }
            node->pointCount--;
            return 1;   /* found and removed */
        }
    }

    /* Recurse into children */
    if (node->divided) {
        if (qt_node_remove(node->nw, type, entityID)) return 1;
        if (qt_node_remove(node->ne, type, entityID)) return 1;
        if (qt_node_remove(node->sw, type, entityID)) return 1;
        if (qt_node_remove(node->se, type, entityID)) return 1;
    }

    return 0;   /* not found in this subtree */
}

/* ── Internal: recursive display ─────────────────────────────────────── */

static void qt_node_display(const QTNode *node, int depth) {
    if (!node) return;

    /* Print stored points */
    for (int i = 0; i < node->pointCount; i++) {
        /* indent based on depth */
        for (int d = 0; d < depth; d++) printf("  ");

        const char *typeStr;
        switch (node->points[i].entityType) {
            case QT_ENTITY_VEHICLE:  typeStr = "Vehicle";  break;
            case QT_ENTITY_FACILITY: typeStr = "Facility"; break;
            case QT_ENTITY_LOCATION: typeStr = "Location"; break;
            default:                 typeStr = "Unknown";  break;
        }
        printf("  %-10s ID=%2d  (%.1f, %.1f)\n",
               typeStr,
               node->points[i].entityID,
               node->points[i].x,
               node->points[i].y);
    }

    /* Recurse into children */
    if (node->divided) {
        qt_node_display(node->nw, depth + 1);
        qt_node_display(node->ne, depth + 1);
        qt_node_display(node->sw, depth + 1);
        qt_node_display(node->se, depth + 1);
    }
}

/* ── Internal: recursive free ────────────────────────────────────────── */

static void qt_node_free(QTNode *node) {
    if (!node) return;
    qt_node_free(node->nw);
    qt_node_free(node->ne);
    qt_node_free(node->sw);
    qt_node_free(node->se);
    free(node);
}

/* ═══════════════════════════════════════════════════════════════════════
 *  Public API
 * ═══════════════════════════════════════════════════════════════════════ */

/* ── Initialise ──────────────────────────────────────────────────────── */

void quadtree_init(QuadTree *qt, QTBoundary boundary) {
    qt->root = qt_node_create(boundary);
    qt->totalPoints = 0;
}

/* ── Insert ──────────────────────────────────────────────────────────── */

int quadtree_insert(QuadTree *qt, QTPoint point) {
    if (!qt->root) return -1;

    if (qt_node_insert(qt->root, point)) {
        qt->totalPoints++;
        return 0;
    }

    fprintf(stderr,
            "[QuadTree] Warning: Point (%.1f, %.1f) outside boundary.\n",
            point.x, point.y);
    return -1;
}

/* ── Range query ─────────────────────────────────────────────────────── */

int quadtree_query_range(const QuadTree *qt, QTBoundary range,
                         QTPoint results[], int maxResults) {
    if (!qt->root) return 0;
    return qt_node_query_range(qt->root, &range, results, maxResults, 0);
}

/* ── Nearest-point query ─────────────────────────────────────────────── */

int quadtree_query_nearest(const QuadTree *qt, double x, double y,
                           QTPoint *nearest) {
    if (!qt->root || qt->totalPoints == 0) return -1;

    double bestDistSq = 1e18;  /* start with a very large distance */
    qt_node_query_nearest(qt->root, x, y, nearest, &bestDistSq);

    return (bestDistSq < 1e18) ? 0 : -1;
}

/* ── Nearest-point query by type ─────────────────────────────────────── */

int quadtree_query_nearest_by_type(const QuadTree *qt, double x, double y,
                                   QTEntityType type, int subType,
                                   QTPoint *nearest) {
    if (!qt->root || qt->totalPoints == 0) return -1;

    double bestDistSq = 1e18;
    qt_node_query_nearest_by_type(qt->root, x, y, type, subType, nearest, &bestDistSq);

    return (bestDistSq < 1e18) ? 0 : -1;
}

/* ── Remove ──────────────────────────────────────────────────────────── */

int quadtree_remove(QuadTree *qt, QTEntityType type, int entityID) {
    if (!qt->root) return -1;

    if (qt_node_remove(qt->root, type, entityID)) {
        qt->totalPoints--;
        return 0;
    }
    return -1;
}

/* ── Display ─────────────────────────────────────────────────────────── */

void quadtree_display(const QuadTree *qt) {
    printf("\n===== Quad Tree (%d points) =====\n", qt->totalPoints);
    if (qt->root) {
        printf("  Root boundary: center(%.0f, %.0f)  size(%.0f x %.0f)\n",
               qt->root->boundary.x_center, qt->root->boundary.y_center,
               qt->root->boundary.half_width * 2,
               qt->root->boundary.half_height * 2);
        qt_node_display(qt->root, 0);
    }
    printf("=================================\n");
}

/* ── Free ────────────────────────────────────────────────────────────── */

void quadtree_free(QuadTree *qt) {
    qt_node_free(qt->root);
    qt->root = NULL;
    qt->totalPoints = 0;
}

/* ── Load vehicles from AVL tree (in-order traversal) ────────────────── */

static void qt_avl_inorder_load(QuadTree *qt, AVLNode *node) {
    if (!node) return;

    qt_avl_inorder_load(qt, node->left);

    QTPoint p;
    p.x          = node->vehicle.position.x;
    p.y          = node->vehicle.position.y;
    p.entityType = QT_ENTITY_VEHICLE;
    p.entityID   = node->vehicle.id;
    p.entitySubType = node->vehicle.type;

    quadtree_insert(qt, p);

    qt_avl_inorder_load(qt, node->right);
}

void quadtree_load_vehicles(QuadTree *qt, AVLNode *root) {
    qt_avl_inorder_load(qt, root);
    printf("[QuadTree] Loaded %d vehicle(s) into quad tree.\n",
           qt->totalPoints);
}
