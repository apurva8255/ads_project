/*
 * ============================================================================
 *  Emergency Response Dispatch & Optimal Routing System
 *  graph.c — City Road Network (Adjacency List Graph) implementation
 * ============================================================================
 */

#include "graph.h"

/* ── Initialise an empty graph ────────────────────────────────────────── */

void graph_init(Graph *g) {
    g->nodeCount = 0;
    for (int i = 0; i < MAX_NODES; i++) {
        g->adjList[i] = NULL;
        g->nodes[i].id = -1;
        g->nodes[i].name[0] = '\0';
        g->nodes[i].position.x = 0.0;
        g->nodes[i].position.y = 0.0;
    }
}

/* ── Add a location node ──────────────────────────────────────────────── */

int graph_add_node(Graph *g, const char *name, double x, double y) {
    if (g->nodeCount >= MAX_NODES) {
        fprintf(stderr, "[Graph] Error: Maximum node limit (%d) reached.\n",
                MAX_NODES);
        return -1;
    }

    int id = g->nodeCount;
    g->nodes[id].id = id;
    strncpy(g->nodes[id].name, name, MAX_NAME_LEN - 1);
    g->nodes[id].name[MAX_NAME_LEN - 1] = '\0';
    g->nodes[id].position.x = x;
    g->nodes[id].position.y = y;

    g->nodeCount++;
    return id;
}

/* ── Internal: create a new edge node ─────────────────────────────────── */

static Edge *create_edge(int destination, double weight) {
    Edge *e = (Edge *)malloc(sizeof(Edge));
    if (!e) {
        fprintf(stderr, "[Graph] Error: Memory allocation failed for edge.\n");
        return NULL;
    }
    e->destination = destination;
    e->weight      = weight;
    e->next        = NULL;
    return e;
}

/* ── Add a bidirectional road ─────────────────────────────────────────── */

int graph_add_edge(Graph *g, int src, int dest, double weight) {
    if (src < 0 || src >= g->nodeCount || dest < 0 || dest >= g->nodeCount) {
        fprintf(stderr,
                "[Graph] Error: Invalid node IDs (%d -> %d). Valid range: 0..%d\n",
                src, dest, g->nodeCount - 1);
        return -1;
    }
    if (weight < 0) {
        fprintf(stderr, "[Graph] Error: Negative weight not allowed.\n");
        return -1;
    }

    /* src -> dest */
    Edge *e1 = create_edge(dest, weight);
    if (!e1) return -1;
    e1->next = g->adjList[src];
    g->adjList[src] = e1;

    /* dest -> src  (bidirectional) */
    Edge *e2 = create_edge(src, weight);
    if (!e2) return -1;
    e2->next = g->adjList[dest];
    g->adjList[dest] = e2;

    return 0;
}

/* ── Find node ID by name ─────────────────────────────────────────────── */

int graph_find_node(Graph *g, const char *name) {
    for (int i = 0; i < g->nodeCount; i++) {
        if (strcmp(g->nodes[i].name, name) == 0) {
            return i;
        }
    }
    return -1;
}

/* ── Display the full city map ────────────────────────────────────────── */

void graph_display(const Graph *g) {
    printf("\n===== City Road Network (%d locations) =====\n", g->nodeCount);
    printf("  (Roads with weight >= 12 are marked [TRAFFIC])\n\n");
    for (int i = 0; i < g->nodeCount; i++) {
        printf("  [%2d] %-15s (%.0f, %.0f)\n",
               g->nodes[i].id,
               g->nodes[i].name,
               g->nodes[i].position.x,
               g->nodes[i].position.y);

        Edge *e = g->adjList[i];
        while (e) {
            if (e->weight >= 12.0) {
                printf("        --> %-15s  %5.1f min  [TRAFFIC]\n",
                       g->nodes[e->destination].name, e->weight);
            } else {
                printf("        --> %-15s  %5.1f min\n",
                       g->nodes[e->destination].name, e->weight);
            }
            e = e->next;
        }
    }
    printf("\n=============================================\n");
}

/* ── Free all dynamically allocated edges ─────────────────────────────── */

void graph_free(Graph *g) {
    for (int i = 0; i < MAX_NODES; i++) {
        Edge *e = g->adjList[i];
        while (e) {
            Edge *tmp = e;
            e = e->next;
            free(tmp);
        }
        g->adjList[i] = NULL;
    }
    g->nodeCount = 0;
}

/* ── Load city map from a file ────────────────────────────────────────── */

int graph_load_from_file(Graph *g, const char *filename) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        fprintf(stderr, "[Graph] Error: Cannot open file '%s'.\n", filename);
        return -1;
    }

    char token[MAX_NAME_LEN];
    int count;

    /* ── Read NODES section ────────────────────────────────────────── */
    if (fscanf(fp, "%s %d", token, &count) != 2 ||
        strcmp(token, "NODES") != 0) {
        fprintf(stderr, "[Graph] Error: Expected 'NODES <count>' header.\n");
        fclose(fp);
        return -1;
    }

    for (int i = 0; i < count; i++) {
        int id;
        char name[MAX_NAME_LEN];
        double x, y;

        if (fscanf(fp, "%d %s %lf %lf", &id, name, &x, &y) != 4) {
            fprintf(stderr, "[Graph] Error: Bad node entry at line %d.\n", i);
            fclose(fp);
            return -1;
        }
        int added = graph_add_node(g, name, x, y);
        if (added < 0) {
            fclose(fp);
            return -1;
        }
    }

    /* ── Read EDGES section ────────────────────────────────────────── */
    if (fscanf(fp, "%s %d", token, &count) != 2 ||
        strcmp(token, "EDGES") != 0) {
        fprintf(stderr, "[Graph] Error: Expected 'EDGES <count>' header.\n");
        fclose(fp);
        return -1;
    }

    for (int i = 0; i < count; i++) {
        int src, dest;
        double weight;

        if (fscanf(fp, "%d %d %lf", &src, &dest, &weight) != 3) {
            fprintf(stderr, "[Graph] Error: Bad edge entry at line %d.\n", i);
            fclose(fp);
            return -1;
        }
        if (graph_add_edge(g, src, dest, weight) < 0) {
            fclose(fp);
            return -1;
        }
    }

    fclose(fp);
    printf("[Graph] Loaded %d nodes and %d edges from '%s'.\n",
           g->nodeCount, count, filename);
    return 0;
}

/* ── Load a predefined hardcoded city map ──────────────────────────────── */

int graph_load_predefined(Graph *g) {

    /* ── 10 Pune city locations ────────────────────────────────────── */
    graph_add_node(g, "Shivajinagar",  30.0, 70.0);   /* 0  */
    graph_add_node(g, "Deccan",        25.0, 60.0);   /* 1  */
    graph_add_node(g, "Swargate",      35.0, 50.0);   /* 2  */
    graph_add_node(g, "Katraj",        40.0, 35.0);   /* 3  */
    graph_add_node(g, "Kondhwa",       50.0, 40.0);   /* 4  */
    graph_add_node(g, "Hadapsar",      60.0, 45.0);   /* 5  */
    graph_add_node(g, "Kharadi",       70.0, 60.0);   /* 6  */
    graph_add_node(g, "Koregaon_Park", 45.0, 65.0);   /* 7  */
    graph_add_node(g, "Pimpri",        20.0, 80.0);   /* 8  */
    graph_add_node(g, "Hinjewadi",     10.0, 75.0);   /* 9  */

    /* ── Roads (bidirectional edges) ───────────────────────────────
     *   Normal roads  : weight  3 – 7  (minutes)
     *   Traffic roads  : weight 12 – 18 (minutes)  marked with [TRAFFIC]
     * ──────────────────────────────────────────────────────────────── */

    graph_add_edge(g, 0, 1,  5.0);    /* Shivajinagar  <-> Deccan          */
    graph_add_edge(g, 0, 7,  7.0);    /* Shivajinagar  <-> Koregaon Park   */
    graph_add_edge(g, 0, 8,  8.0);    /* Shivajinagar  <-> Pimpri          */
    graph_add_edge(g, 1, 2,  6.0);    /* Deccan        <-> Swargate        */
    graph_add_edge(g, 1, 7,  4.0);    /* Deccan        <-> Koregaon Park   */
    graph_add_edge(g, 2, 3, 15.0);    /* Swargate      <-> Katraj     [TRAFFIC] */
    graph_add_edge(g, 2, 7,  3.0);    /* Swargate      <-> Koregaon Park   */
    graph_add_edge(g, 3, 4,  5.0);    /* Katraj        <-> Kondhwa         */
    graph_add_edge(g, 4, 5,  6.0);    /* Kondhwa       <-> Hadapsar        */
    graph_add_edge(g, 5, 6,  3.0);    /* Hadapsar      <-> Kharadi         */
    graph_add_edge(g, 6, 7, 18.0);    /* Kharadi       <-> Koregaon Park  [TRAFFIC] */
    graph_add_edge(g, 7, 5,  7.0);    /* Koregaon Park <-> Hadapsar        */
    graph_add_edge(g, 8, 9,  6.0);    /* Pimpri        <-> Hinjewadi       */
    graph_add_edge(g, 9, 1, 14.0);    /* Hinjewadi     <-> Deccan     [TRAFFIC] */
    graph_add_edge(g, 4, 7,  9.0);    /* Kondhwa       <-> Koregaon Park   */

    printf("[Graph] Predefined city map loaded: %d locations, 15 roads.\n",
           g->nodeCount);
    printf("        Traffic-heavy routes: Swargate<->Katraj, ");
    printf("Kharadi<->Koregaon_Park, Hinjewadi<->Deccan\n");
    return 0;
}
