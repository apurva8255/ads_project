/*
 * ============================================================================
 *  Emergency Response Dispatch & Optimal Routing System
 *  dijkstra.h — Shortest Path Algorithm
 * ============================================================================
 */

#ifndef DIJKSTRA_H
#define DIJKSTRA_H

#include "graph.h"
#include "fib_heap.h"

/* ── Shortest-path result ─────────────────────────────────────────────── */

typedef struct {
    double distance;               /* total shortest distance              */
    int    path[MAX_NODES];        /* list of node IDs from src to dest    */
    int    pathLength;             /* number of nodes in the path          */
} DijkstraResult;

/* ── Public API ───────────────────────────────────────────────────────── */

/*
 * Compute the shortest path from `source` to `destination` in graph `g`.
 * Fills in `result` with the distance and the full node-by-node path.
 * Returns 0 on success, -1 if no path exists or inputs are invalid.
 */
int dijkstra_shortest_path(const Graph *g, int source, int destination,
                           DijkstraResult *result);

/*
 * Compute shortest distances from `source` to ALL other nodes.
 * `dist` must be an array of at least g->nodeCount doubles.
 * `prev` must be an array of at least g->nodeCount ints (predecessor IDs).
 * Returns 0 on success.
 */
int dijkstra_all_distances(const Graph *g, int source,
                           double dist[], int prev[]);

/* Print a DijkstraResult nicely */
void dijkstra_print_result(const Graph *g, const DijkstraResult *result,
                           int source, int destination);

#endif /* DIJKSTRA_H */
