/*
 * ============================================================================
 *  Emergency Response Dispatch & Optimal Routing System
 *  dijkstra.c — Shortest Path Algorithm (using Fibonacci Heap)
 * ============================================================================
 */

#include "dijkstra.h"

/* ── Build path from predecessor array ────────────────────────────────── */

static void build_path(const int prev[], int destination,
                       DijkstraResult *result) {
    int stack[MAX_NODES];
    int top = 0;
    int cur = destination;

    while (cur != -1) {
        stack[top++] = cur;
        cur = prev[cur];
    }

    result->pathLength = top;
    for (int i = 0; i < top; i++) {
        result->path[i] = stack[top - 1 - i];
    }
}

/* ── Dijkstra: single source → all distances (Fibonacci Heap) ─────────── */

int dijkstra_all_distances(const Graph *g, int source,
                           double dist[], int prev[]) {
    if (!g || source < 0 || source >= g->nodeCount) {
        fprintf(stderr, "[Dijkstra] Error: Invalid source node %d.\n", source);
        return -1;
    }

    int visited[MAX_NODES] = {0};
    FibNode *nodeRef[MAX_NODES];  /* pointers to Fib nodes for decrease-key */

    for (int i = 0; i < g->nodeCount; i++) {
        dist[i] = INF;
        prev[i] = -1;
        nodeRef[i] = NULL;
    }
    dist[source] = 0.0;

    /* Create Fibonacci Heap and insert source */
    FibHeap *heap = fib_heap_create();
    if (!heap) return -1;

    nodeRef[source] = fib_heap_insert(heap, source, 0.0);

    /* Insert all other nodes with INF */
    for (int i = 0; i < g->nodeCount; i++) {
        if (i != source) {
            nodeRef[i] = fib_heap_insert(heap, i, INF);
        }
    }

    while (!fib_heap_is_empty(heap)) {
        FibNode *minNode = fib_heap_extract_min(heap);
        if (!minNode) break;

        int u = minNode->id;
        double uDist = minNode->key;
        free(minNode);

        if (uDist >= INF) break;  /* remaining nodes are unreachable */
        if (visited[u]) continue;
        visited[u] = 1;
        nodeRef[u] = NULL;

        /* relax all neighbors */
        Edge *e = g->adjList[u];
        while (e) {
            int v = e->destination;
            double newDist = dist[u] + e->weight;

            if (!visited[v] && newDist < dist[v]) {
                dist[v] = newDist;
                prev[v] = u;
                /* decrease-key in the Fibonacci Heap */
                if (nodeRef[v]) {
                    fib_heap_decrease_key(heap, nodeRef[v], newDist);
                }
            }
            e = e->next;
        }
    }

    fib_heap_free(heap);
    return 0;
}

/* ── Dijkstra: source → destination ───────────────────────────────────── */

int dijkstra_shortest_path(const Graph *g, int source, int destination,
                           DijkstraResult *result) {
    if (!g || !result) return -1;
    if (source < 0 || source >= g->nodeCount ||
        destination < 0 || destination >= g->nodeCount) {
        fprintf(stderr,
                "[Dijkstra] Error: Invalid node IDs (src=%d, dest=%d).\n",
                source, destination);
        return -1;
    }

    double dist[MAX_NODES];
    int    prev[MAX_NODES];

    if (dijkstra_all_distances(g, source, dist, prev) < 0)
        return -1;

    if (dist[destination] >= INF) {
        fprintf(stderr, "[Dijkstra] No path from '%s' to '%s'.\n",
                g->nodes[source].name, g->nodes[destination].name);
        result->distance   = -1;
        result->pathLength = 0;
        return -1;
    }

    result->distance = dist[destination];
    build_path(prev, destination, result);
    return 0;
}

/* ── Pretty-print a DijkstraResult ────────────────────────────────────── */

void dijkstra_print_result(const Graph *g, const DijkstraResult *result,
                           int source, int destination) {
    if (!result || result->pathLength == 0) {
        printf("  No path from [%d] to [%d].\n", source, destination);
        return;
    }

    printf("\n--- Shortest Path: %s -> %s ---\n",
           g->nodes[source].name, g->nodes[destination].name);
    printf("  Distance : %.2f min\n", result->distance);
    printf("  Route    : ");

    for (int i = 0; i < result->pathLength; i++) {
        printf("%s", g->nodes[result->path[i]].name);
        if (i < result->pathLength - 1) printf(" -> ");
    }
    printf("\n");
    printf("--------------------------------------\n");
}
