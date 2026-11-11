/*
 * ============================================================================
 *  Emergency Response Dispatch & Optimal Routing System
 *  graph.h — City Road Network (Adjacency List Graph)
 * ============================================================================
 */

#ifndef GRAPH_H
#define GRAPH_H

#include "common.h"

/* ── Edge in the adjacency list ───────────────────────────────────────── */

typedef struct Edge {
    int    destination;    /* node ID of the other end          */
    double weight;         /* travel time / distance            */
    struct Edge *next;     /* next edge in the adjacency list   */
} Edge;

/* ── Graph  ───────────────────────────────────────────────────────────── */

typedef struct {
    GraphNode nodes[MAX_NODES];    /* array of city locations            */
    Edge     *adjList[MAX_NODES];  /* adjacency list head for each node  */
    int       nodeCount;           /* number of locations currently in   */
} Graph;

/* ── Public API ───────────────────────────────────────────────────────── */

/* Initialise an empty graph */
void graph_init(Graph *g);

/* Add a location node; returns the assigned ID, or -1 on failure */
int graph_add_node(Graph *g, const char *name, double x, double y);

/* Add a bidirectional road between two node IDs with given weight */
int graph_add_edge(Graph *g, int src, int dest, double weight);

/* Find a node ID by name; returns -1 if not found */
int graph_find_node(Graph *g, const char *name);

/* Display the full city map (all nodes + their edges) */
void graph_display(const Graph *g);

/* Free dynamically allocated edge lists */
void graph_free(Graph *g);

/* Load city map from a file.  Returns 0 on success, -1 on error.
 *
 * File format:
 *   NODES <count>
 *   <id> <name> <x> <y>
 *   ...
 *   EDGES <count>
 *   <srcID> <destID> <weight>
 *   ...
 */
int graph_load_from_file(Graph *g, const char *filename);

/*
 * Load a predefined hardcoded city map (10 Pune locations).
 * Some roads include traffic (higher weights).
 * Returns 0 on success.
 */
int graph_load_predefined(Graph *g);

#endif /* GRAPH_H */
