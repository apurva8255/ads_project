/*
 * ============================================================================
 *  Emergency Response Dispatch & Optimal Routing System
 *  api.h — DLL Export API for Python ctypes bridge
 * ============================================================================
 *
 *  This header exposes a flat C API suitable for FFI (Foreign Function Interface).
 *  All functions use simple types (int, double, char*) and flat structs
 *  so Python ctypes can marshal them easily.
 *
 *  Build as shared library:
 *    gcc -shared -o emergency_engine.dll -Iinclude src/*.c -lm
 */

#ifndef API_H
#define API_H

#ifdef _WIN32
  #define API_EXPORT __declspec(dllexport)
#else
  #define API_EXPORT __attribute__((visibility("default")))
#endif

/* ── Flat response structs for ctypes ────────────────────────────────── */

/* Location match (Trie autocomplete result) */
typedef struct {
    int  nodeID;
    char name[64];
} APITrieMatch;

/* Vehicle record (from AVL Tree) */
typedef struct {
    int    id;
    int    type;       /* 0=Ambulance, 1=Police, 2=Fire */
    int    locationNodeID;
    double posX;
    double posY;
    int    status;     /* 0=Available, 1=Busy */
    char   name[64];
} APIVehicle;

/* Facility record (from B+ Tree) */
typedef struct {
    int    id;
    int    type;       /* 0=Hospital, 1=Police, 2=Fire */
    int    locationNodeID;
    double posX;
    double posY;
    int    capacity;
    char   name[64];
} APIFacility;

/* Emergency call record (from Priority Queue) */
typedef struct {
    int  callID;
    int  type;         /* 0=Medical, 1=Crime, 2=Fire */
    char locationName[64];
    int  locationNodeID;
    int  timestamp;
    int  priority;     /* 1=Critical, 2=High, 3=Normal */
} APIEmergencyCall;

/* Route step (from Dijkstra) */
typedef struct {
    int    nodeID;
    char   name[64];
    double posX;
    double posY;
} APIRouteStep;

/* Dispatch result (combined QuadTree + Dijkstra + AVL + B+Tree) */
typedef struct {
    /* Emergency info */
    int  callID;
    int  emergencyType;
    char locationName[64];
    int  destNodeID;

    /* Vehicle assigned */
    int    vehicleID;
    char   vehicleName[64];
    int    vehicleType;
    int    vehicleNodeID;

    /* Facility routed to */
    int    facilityID;
    char   facilityName[64];
    int    facilityType;
    int    facilityNodeID;
    int    facilityCapacity;

    /* Vehicle → Emergency route */
    double vehicleRouteDistance;
    int    vehicleRoutePath[500];
    int    vehicleRouteLength;

    /* Emergency → Facility route */
    double facilityRouteDistance;
    int    facilityRoutePath[500];
    int    facilityRouteLength;

    /* Status */
    int    success;    /* 1=ok, 0=failed */
    char   error[128]; /* error message if failed */
} APIDispatchResult;

/* ── API Functions ───────────────────────────────────────────────────── */

#ifdef __cplusplus
extern "C" {
#endif

/* Initialize the engine — load all data files and build all data structures.
 * Must be called once before any other API function.
 * Returns 0 on success, -1 on failure. */
API_EXPORT int api_init(const char *dataDir);

/* Shutdown the engine — free all memory */
API_EXPORT void api_shutdown(void);

/* ── Trie: Location Search ───────────────────────────────────────────── */

/* Search for locations by prefix. Returns number of matches (up to maxResults). */
API_EXPORT int api_search_prefix(const char *prefix, APITrieMatch *results, int maxResults);

/* Search for exact location name. Returns nodeID, or -1 if not found. */
API_EXPORT int api_search_exact(const char *name);

/* ── Queue: Emergency Management ─────────────────────────────────────── */

/* Add an emergency to the queue. Returns callID on success, -1 on failure. */
API_EXPORT int api_add_emergency(int emergencyType, const char *locationName,
                                  int priority);

/* Get all emergencies currently in the queue. Returns count. */
API_EXPORT int api_get_queue(APIEmergencyCall *results, int maxResults);

/* Get the number of emergencies in the queue. */
API_EXPORT int api_get_queue_count(void);

/* ── Dispatch: Full Orchestration ────────────────────────────────────── */

/* Process the highest-priority emergency: dequeue, find nearest vehicle,
 * compute routes, mark vehicle busy. Returns result struct. */
API_EXPORT APIDispatchResult api_dispatch(void);

/* ── AVL Tree: Vehicle Database ──────────────────────────────────────── */

/* Get all vehicles. Returns count. */
API_EXPORT int api_get_vehicles(APIVehicle *results, int maxResults);

/* Get vehicle count */
API_EXPORT int api_get_vehicle_count(void);

/* Resolve (free) a vehicle by ID. Returns 0 on success, -1 if not found. */
API_EXPORT int api_resolve_vehicle(int vehicleID);

/* ── B+ Tree: Facility Database ──────────────────────────────────────── */

/* Get all facilities. Returns count. */
API_EXPORT int api_get_facilities(APIFacility *results, int maxResults);

/* Get facility count */
API_EXPORT int api_get_facility_count(void);

/* ── Dijkstra: Route Computation ─────────────────────────────────────── */

/* Compute shortest path between two nodes.
 * Fills `path` with node IDs and returns path length.
 * Sets *distance to total distance. Returns -1 on failure. */
API_EXPORT int api_get_route(int fromNode, int toNode,
                              int *path, int maxPath, double *distance);

/* Get a route step (node info) by node ID */
API_EXPORT APIRouteStep api_get_node_info(int nodeID);

/* ── Graph Info ──────────────────────────────────────────────────────── */

/* Get the total number of nodes in the city graph */
API_EXPORT int api_get_node_count(void);

/* Get the total number of edges in the city graph */
API_EXPORT int api_get_edge_count(void);

#ifdef __cplusplus
}
#endif

#endif /* API_H */
