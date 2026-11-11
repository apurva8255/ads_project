/*
 * ============================================================================
 *  Emergency Response Dispatch & Optimal Routing System
 *  api.c — DLL Export API Implementation
 * ============================================================================
 *
 *  This file wraps all internal data structures into a flat C API that
 *  Python ctypes can call. It mirrors the logic from main.c but exposes
 *  it via exported functions instead of a CLI menu.
 */

#include "api.h"
#include "avl_tree.h"
#include "bplus_tree.h"
#include "common.h"
#include "dijkstra.h"
#include "graph.h"
#include "quadtree.h"
#include "queue.h"
#include "trie.h"

#include <stdio.h>
#include <string.h>

/* ── Global data structures (same as main.c) ────────────────────────── */
static Graph         g_cityMap;
static AVLTree       g_vehicleDB;
static Trie          g_locationDB;
static QuadTree      g_spatialDB;
static BPTree        g_facilityDB;
static EmergencyQueue g_callQueue;
static int           g_initialized = 0;
static int           g_virtualClock = 1000;

/* ── Helper: collect vehicles from AVL via in-order traversal ────────── */
static int avl_collect_helper(AVLNode *node, APIVehicle *results,
                               int maxResults, int *count) {
    if (!node || *count >= maxResults) return 0;
    avl_collect_helper(node->left, results, maxResults, count);
    if (*count < maxResults) {
        Vehicle *v = &node->vehicle;
        results[*count].id = v->id;
        results[*count].type = (int)v->type;
        results[*count].locationNodeID = v->locationNodeID;
        results[*count].posX = v->position.x;
        results[*count].posY = v->position.y;
        results[*count].status = (int)v->status;
        strncpy(results[*count].name, v->name, 63);
        results[*count].name[63] = '\0';
        (*count)++;
    }
    avl_collect_helper(node->right, results, maxResults, count);
    return *count;
}

/* ── Helper: collect queue items without modifying the queue ──────────── */
static int queue_collect(EmergencyQueue *q, APIEmergencyCall *results,
                          int maxResults) {
    int count = 0;
    QueueNode *curr = q->head;
    while (curr && count < maxResults) {
        EmergencyCall *c = &curr->call;
        results[count].callID = c->callID;
        results[count].type = (int)c->type;
        strncpy(results[count].locationName, c->locationName, 63);
        results[count].locationName[63] = '\0';
        results[count].locationNodeID = c->locationNodeID;
        results[count].timestamp = c->timestamp;
        results[count].priority = c->priority;
        count++;
        curr = curr->next;
    }
    return count;
}

/* ═══════════════════════════════════════════════════════════════════════ */
/*                        EXPORTED API FUNCTIONS                         */
/* ═══════════════════════════════════════════════════════════════════════ */

API_EXPORT int api_init(const char *dataDir) {
    if (g_initialized) return 0; /* already initialized */

    char path[256];

    /* ── Load city map ── */
    graph_init(&g_cityMap);
    snprintf(path, sizeof(path), "%s/city_map.txt", dataDir);
    if (graph_load_from_file(&g_cityMap, path) < 0) {
        fprintf(stderr, "[API] Failed to load city map from %s\n", path);
        return -1;
    }

    /* ── Load vehicles into AVL tree ── */
    avl_init(&g_vehicleDB);
    snprintf(path, sizeof(path), "%s/vehicles.txt", dataDir);
    if (avl_load_from_file(&g_vehicleDB, path) < 0) {
        fprintf(stderr, "[API] Failed to load vehicles from %s\n", path);
        graph_free(&g_cityMap);
        return -1;
    }

    /* ── Build location Trie from graph ── */
    trie_init(&g_locationDB);
    trie_load_from_graph(&g_locationDB, &g_cityMap);

    /* ── Build QuadTree spatial index ── */
    QTBoundary cityBounds = qt_boundary_create(
        CITY_WIDTH / 2.0, CITY_HEIGHT / 2.0,
        CITY_WIDTH / 2.0, CITY_HEIGHT / 2.0);
    quadtree_init(&g_spatialDB, cityBounds);
    quadtree_load_vehicles(&g_spatialDB, g_vehicleDB.root);

    /* ── Load facilities into B+ tree ── */
    bptree_init(&g_facilityDB);
    snprintf(path, sizeof(path), "%s/facilities.txt", dataDir);
    if (bptree_load_from_file(&g_facilityDB, path, &g_spatialDB) < 0) {
        fprintf(stderr, "[API] Failed to load facilities from %s\n", path);
    }

    /* ── Inject facilities into Trie for autocomplete ── */
    {
        Facility facs[MAX_FACILITIES];
        int count = bptree_range_query(&g_facilityDB, 0, 999999,
                                        facs, MAX_FACILITIES);
        for (int i = 0; i < count; i++) {
            trie_insert(&g_locationDB, facs[i].name, facs[i].locationNodeID);
        }
    }

    /* ── Initialize emergency queue ── */
    queue_init(&g_callQueue);

    g_initialized = 1;
    printf("[API] Engine initialized: %d nodes, %d vehicles, %d facilities\n",
           g_cityMap.nodeCount, g_vehicleDB.count, g_facilityDB.count);
    return 0;
}

API_EXPORT void api_shutdown(void) {
    if (!g_initialized) return;
    graph_free(&g_cityMap);
    avl_free(&g_vehicleDB);
    trie_free(&g_locationDB);
    quadtree_free(&g_spatialDB);
    bptree_free(&g_facilityDB);
    queue_free(&g_callQueue);
    g_initialized = 0;
}

/* ── Trie: Location Search ──────────────────────────────────────────── */

API_EXPORT int api_search_prefix(const char *prefix, APITrieMatch *results,
                                  int maxResults) {
    if (!g_initialized) return 0;
    TrieMatch matches[50];
    int count = trie_search_prefix(&g_locationDB, prefix, matches,
                                    maxResults < 50 ? maxResults : 50);
    for (int i = 0; i < count && i < maxResults; i++) {
        results[i].nodeID = matches[i].nodeID;
        strncpy(results[i].name, matches[i].name, 63);
        results[i].name[63] = '\0';
    }
    return count;
}

API_EXPORT int api_search_exact(const char *name) {
    if (!g_initialized) return -1;
    return trie_search_exact(&g_locationDB, name);
}

/* ── Queue: Emergency Management ────────────────────────────────────── */

API_EXPORT int api_add_emergency(int emergencyType, const char *locationName,
                                  int priority) {
    if (!g_initialized) return -1;
    int result = queue_enqueue(&g_callQueue, (EmergencyType)emergencyType,
                                locationName, priority, g_virtualClock++);
    if (result == 0) {
        /* Return the callID of the last enqueued item */
        /* Walk to find the call we just added */
        QueueNode *curr = g_callQueue.head;
        int lastID = -1;
        while (curr) {
            lastID = curr->call.callID;
            curr = curr->next;
        }
        return lastID;
    }
    return -1;
}

API_EXPORT int api_get_queue(APIEmergencyCall *results, int maxResults) {
    if (!g_initialized) return 0;
    return queue_collect(&g_callQueue, results, maxResults);
}

API_EXPORT int api_get_queue_count(void) {
    if (!g_initialized) return 0;
    return g_callQueue.count;
}

/* ── Dispatch: Full Orchestration ───────────────────────────────────── */

API_EXPORT APIDispatchResult api_dispatch(void) {
    APIDispatchResult res;
    memset(&res, 0, sizeof(res));

    if (!g_initialized) {
        res.success = 0;
        strncpy(res.error, "Engine not initialized", 127);
        return res;
    }

    /* Dequeue highest-priority emergency */
    EmergencyCall call;
    if (!queue_dequeue(&g_callQueue, &call)) {
        res.success = 0;
        strncpy(res.error, "Queue is empty", 127);
        return res;
    }

    res.callID = call.callID;
    res.emergencyType = (int)call.type;
    strncpy(res.locationName, call.locationName, 63);

    /* Resolve location to graph node */
    int destNode = trie_search_exact(&g_locationDB, call.locationName);
    if (destNode == -1) {
        res.success = 0;
        snprintf(res.error, 127, "Location '%s' not found", call.locationName);
        return res;
    }
    res.destNodeID = destNode;

    double x = g_cityMap.nodes[destNode].position.x;
    double y = g_cityMap.nodes[destNode].position.y;

    VehicleType vType = emergency_to_vehicle_type(call.type);
    FacilityType fType = emergency_to_facility_type(call.type);

    /* Find nearest vehicle via QuadTree */
    QTPoint nearestVeh;
    int vFound = quadtree_query_nearest_by_type(
        &g_spatialDB, x, y, QT_ENTITY_VEHICLE, (int)vType, &nearestVeh);

    /* Find nearest facility via QuadTree */
    QTPoint nearestFac;
    int fFound = quadtree_query_nearest_by_type(
        &g_spatialDB, x, y, QT_ENTITY_FACILITY, (int)fType, &nearestFac);

    /* Look up vehicle in AVL tree */
    Vehicle *v = NULL;
    if (vFound == 0) {
        v = avl_search(&g_vehicleDB, nearestVeh.entityID);
    }

    /* Look up facility in B+ tree */
    Facility *f = NULL;
    if (fFound == 0) {
        f = bptree_search(&g_facilityDB, nearestFac.entityID);
    }

    if (v) {
        res.vehicleID = v->id;
        strncpy(res.vehicleName, v->name, 63);
        res.vehicleType = (int)v->type;
        res.vehicleNodeID = v->locationNodeID;

        /* Compute vehicle → emergency route */
        DijkstraResult vRoute;
        if (dijkstra_shortest_path(&g_cityMap, v->locationNodeID, destNode,
                                    &vRoute) == 0) {
            res.vehicleRouteDistance = vRoute.distance;
            res.vehicleRouteLength = vRoute.pathLength;
            for (int i = 0; i < vRoute.pathLength && i < 500; i++) {
                res.vehicleRoutePath[i] = vRoute.path[i];
            }
        }

        /* Mark vehicle as busy */
        v->status = STATUS_BUSY;
    } else {
        res.success = 0;
        snprintf(res.error, 127, "No available %s found",
                 vehicle_type_str(vType));
        return res;
    }

    if (f) {
        res.facilityID = f->id;
        strncpy(res.facilityName, f->name, 63);
        res.facilityType = (int)f->type;
        res.facilityNodeID = f->locationNodeID;
        res.facilityCapacity = f->capacity;

        /* Compute emergency → facility route */
        DijkstraResult fRoute;
        if (dijkstra_shortest_path(&g_cityMap, destNode, f->locationNodeID,
                                    &fRoute) == 0) {
            res.facilityRouteDistance = fRoute.distance;
            res.facilityRouteLength = fRoute.pathLength;
            for (int i = 0; i < fRoute.pathLength && i < 500; i++) {
                res.facilityRoutePath[i] = fRoute.path[i];
            }
        }
    }

    res.success = 1;
    return res;
}

/* ── AVL Tree: Vehicle Database ─────────────────────────────────────── */

API_EXPORT int api_get_vehicles(APIVehicle *results, int maxResults) {
    if (!g_initialized) return 0;
    int count = 0;
    avl_collect_helper(g_vehicleDB.root, results, maxResults, &count);
    return count;
}

API_EXPORT int api_get_vehicle_count(void) {
    if (!g_initialized) return 0;
    return g_vehicleDB.count;
}

API_EXPORT int api_resolve_vehicle(int vehicleID) {
    if (!g_initialized) return -1;
    Vehicle *v = avl_search(&g_vehicleDB, vehicleID);
    if (v) {
        v->status = STATUS_AVAILABLE;
        return 0;
    }
    return -1;
}

/* ── B+ Tree: Facility Database ─────────────────────────────────────── */

API_EXPORT int api_get_facilities(APIFacility *results, int maxResults) {
    if (!g_initialized) return 0;
    Facility facs[MAX_FACILITIES];
    int count = bptree_range_query(&g_facilityDB, 0, 999999,
                                    facs, MAX_FACILITIES);
    int n = count < maxResults ? count : maxResults;
    for (int i = 0; i < n; i++) {
        results[i].id = facs[i].id;
        results[i].type = (int)facs[i].type;
        results[i].locationNodeID = facs[i].locationNodeID;
        results[i].posX = facs[i].position.x;
        results[i].posY = facs[i].position.y;
        results[i].capacity = facs[i].capacity;
        strncpy(results[i].name, facs[i].name, 63);
        results[i].name[63] = '\0';
    }
    return n;
}

API_EXPORT int api_get_facility_count(void) {
    if (!g_initialized) return 0;
    return g_facilityDB.count;
}

/* ── Dijkstra: Route Computation ────────────────────────────────────── */

API_EXPORT int api_get_route(int fromNode, int toNode,
                              int *path, int maxPath, double *distance) {
    if (!g_initialized) return -1;
    DijkstraResult result;
    if (dijkstra_shortest_path(&g_cityMap, fromNode, toNode, &result) != 0) {
        return -1;
    }
    *distance = result.distance;
    int n = result.pathLength < maxPath ? result.pathLength : maxPath;
    for (int i = 0; i < n; i++) {
        path[i] = result.path[i];
    }
    return n;
}

API_EXPORT APIRouteStep api_get_node_info(int nodeID) {
    APIRouteStep step;
    memset(&step, 0, sizeof(step));
    if (!g_initialized || nodeID < 0 || nodeID >= g_cityMap.nodeCount) {
        return step;
    }
    step.nodeID = nodeID;
    strncpy(step.name, g_cityMap.nodes[nodeID].name, 63);
    step.name[63] = '\0';
    step.posX = g_cityMap.nodes[nodeID].position.x;
    step.posY = g_cityMap.nodes[nodeID].position.y;
    return step;
}

/* ── Graph Info ─────────────────────────────────────────────────────── */

API_EXPORT int api_get_node_count(void) {
    if (!g_initialized) return 0;
    return g_cityMap.nodeCount;
}

API_EXPORT int api_get_edge_count(void) {
    if (!g_initialized) return 0;
    int count = 0;
    for (int i = 0; i < g_cityMap.nodeCount; i++) {
        Edge *e = g_cityMap.adjList[i];
        while (e) {
            count++;
            e = e->next;
        }
    }
    return count / 2; /* edges are bidirectional */
}
