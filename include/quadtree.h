/*
 * ============================================================================
 *  Emergency Response Dispatch & Optimal Routing System
 *  quadtree.h — Quad Tree for Spatial City Partitioning
 * ============================================================================
 */

#ifndef QUADTREE_H
#define QUADTREE_H

#include "common.h"

/* ── Entity type stored in the Quad Tree ─────────────────────────────── */

typedef enum {
    QT_ENTITY_VEHICLE  = 0,
    QT_ENTITY_FACILITY = 1,
    QT_ENTITY_LOCATION = 2
} QTEntityType;

/* ── Point (a spatial entity) ────────────────────────────────────────── */

typedef struct {
    double       x;
    double       y;
    QTEntityType entityType;     /* what kind of entity this point is     */
    int          entityID;       /* ID of the vehicle/facility/location   */
    int          entitySubType;  /* VehicleType or FacilityType           */
} QTPoint;

/* ── Axis-Aligned Bounding Box (region boundary) ─────────────────────── */

typedef struct {
    double x_center;             /* center x of the region                */
    double y_center;             /* center y of the region                */
    double half_width;           /* half the width  (extent in x)         */
    double half_height;          /* half the height (extent in y)         */
} QTBoundary;

/* ── Quad Tree Node ──────────────────────────────────────────────────── */

#define QUADTREE_CAPACITY 4      /* max points before a node splits      */

typedef struct QTNode {
    QTBoundary     boundary;     /* region this node covers               */
    QTPoint        points[QUADTREE_CAPACITY]; /* stored points            */
    int            pointCount;   /* how many points are stored here       */
    int            divided;      /* has this node been subdivided?        */
    struct QTNode *nw;           /* northwest child  (top-left)           */
    struct QTNode *ne;           /* northeast child  (top-right)          */
    struct QTNode *sw;           /* southwest child  (bottom-left)        */
    struct QTNode *se;           /* southeast child  (bottom-right)       */
} QTNode;

/* ── Quad Tree wrapper ───────────────────────────────────────────────── */

typedef struct {
    QTNode *root;
    int     totalPoints;         /* total number of inserted points       */
} QuadTree;

/* ── Public API ──────────────────────────────────────────────────────── */

/*
 * Create a boundary from center and extents.
 */
QTBoundary qt_boundary_create(double cx, double cy,
                              double half_w, double half_h);

/*
 * Check whether a boundary contains a given point.
 */
int qt_boundary_contains(const QTBoundary *b, double x, double y);

/*
 * Check whether two boundaries intersect (overlap).
 */
int qt_boundary_intersects(const QTBoundary *a, const QTBoundary *b);

/*
 * Initialise an empty quad tree covering the given boundary.
 */
void quadtree_init(QuadTree *qt, QTBoundary boundary);

/*
 * Insert a point into the quad tree.
 * Returns 0 on success, -1 if the point is outside the root boundary.
 */
int quadtree_insert(QuadTree *qt, QTPoint point);

/*
 * Query all points within a rectangular range.
 * Fills `results` array (up to `maxResults`).
 * Returns the number of points found.
 */
int quadtree_query_range(const QuadTree *qt, QTBoundary range,
                         QTPoint results[], int maxResults);

/*
 * Find the nearest point to (x, y).
 * Returns 0 on success (fills `nearest`), -1 if tree is empty.
 */
int quadtree_query_nearest(const QuadTree *qt, double x, double y,
                           QTPoint *nearest);

/*
 * Find the nearest point filtered by entityType and entitySubType.
 */
int quadtree_query_nearest_by_type(const QuadTree *qt, double x, double y,
                                   QTEntityType type, int subType,
                                   QTPoint *nearest);

/*
 * Remove a point by matching entityType and entityID.
 * Returns 0 on success, -1 if not found.
 *
 * Note: After removal the tree is NOT rebalanced / compacted.
 *       For our use case (small dataset) this is fine.
 */
int quadtree_remove(QuadTree *qt, QTEntityType type, int entityID);

/*
 * Display all points stored in the quad tree (depth-first traversal).
 */
void quadtree_display(const QuadTree *qt);

/*
 * Free all dynamically allocated nodes.
 */
void quadtree_free(QuadTree *qt);

/*
 * Load all vehicles from an AVL tree into the quad tree.
 * (Convenience function used at startup.)
 * Requires avl_tree.h types; forward-declared here to avoid coupling.
 */
struct AVLNode;   /* forward declaration */
void quadtree_load_vehicles(QuadTree *qt, struct AVLNode *root);

#endif /* QUADTREE_H */
