/*
 * ============================================================================
 *  Emergency Response Dispatch & Optimal Routing System
 *  common.h — Shared types, constants, enums, and structs
 * ============================================================================
 */

#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <float.h>

/* ── Limits & Constants ───────────────────────────────────────────────── */

#define MAX_NODES           100     /* max locations in the city graph       */
#define MAX_VEHICLES        50      /* max emergency vehicles                */
// #define MAX_FACILITIES      50      /* max hospitals/stations                */
#define MAX_NAME_LEN        64      /* max length of a name string           */
#define MAX_EDGES_PER_NODE  20      /* max roads from one location           */
// #define MAX_QUEUE_SIZE      100     /* max pending emergency calls           */
#define TRIE_ALPHABET_SIZE  26      /* lowercase a-z                         */
// #define QUADTREE_CAPACITY   4       /* max points before a quadtree node splits */
// #define BPTREE_ORDER        4       /* B+ tree order (max keys per node + 1) */
#define INF                 1e9     /* represents infinity for Dijkstra      */
#define CITY_WIDTH          100.0   /* city area width  (x: 0 .. 100)       */
#define CITY_HEIGHT         100.0   /* city area height (y: 0 .. 100)       */

/* ── Enums ────────────────────────────────────────────────────────────── */

/* Types of emergency vehicles */
typedef enum {
    VEHICLE_AMBULANCE = 0,
    VEHICLE_POLICE    = 1,
    VEHICLE_FIRE      = 2
} VehicleType;

/* Vehicle availability status */
typedef enum {
    STATUS_AVAILABLE = 0,
    STATUS_BUSY      = 1
} VehicleStatus;

/* 
// Types of service facilities
typedef enum {
    FACILITY_HOSPITAL       = 0,
    FACILITY_POLICE_STATION = 1,
    FACILITY_FIRE_STATION   = 2
} FacilityType;

// Types of emergencies (mirrors vehicle types)
typedef enum {
    EMERGENCY_MEDICAL = 0,   // needs ambulance
    EMERGENCY_CRIME   = 1,   // needs police
    EMERGENCY_FIRE    = 2    // needs fire brigade
} EmergencyType;

// Entity type stored in the Quad Tree
typedef enum {
    ENTITY_VEHICLE  = 0,
    ENTITY_FACILITY = 1
} EntityType;
*/

/* ── Coordinate ───────────────────────────────────────────────────────── */

typedef struct {
    double x;
    double y;
} Coordinate;

/* ── Vehicle Record  (used by AVL Tree) ───────────────────────────────── */

typedef struct {
    int           id;
    VehicleType   type;
    int           locationNodeID;   /* graph node where the vehicle is      */
    Coordinate    position;         /* spatial position for quad-tree       */
    VehicleStatus status;
    char          name[MAX_NAME_LEN];
} Vehicle;

/* ── Facility Record (used by B+ Tree) ────────────────────────────────── */

/*
typedef struct {
    int           id;
    FacilityType  type;
    int           locationNodeID;   // graph node where the facility is
    Coordinate    position;         // spatial position for quad-tree
    int           capacity;
    char          name[MAX_NAME_LEN];
} Facility;
*/

/* ── Emergency Call  (used by Queue) ──────────────────────────────────── */

/*
typedef struct {
    int           callID;
    EmergencyType type;
    char          locationName[MAX_NAME_LEN];
    int           locationNodeID;   // resolved graph node (-1 if unknown)
    int           timestamp;        // simulation time of the call
} EmergencyCall;
*/

/* ── Graph Node (City Location) ───────────────────────────────────────── */

typedef struct {
    int        id;
    char       name[MAX_NAME_LEN];
    Coordinate position;
} GraphNode;

/* ── Utility helpers ──────────────────────────────────────────────────── */

/* Return a descriptive string for a VehicleType enum value */
static inline const char *vehicle_type_str(VehicleType t) {
    switch (t) {
        case VEHICLE_AMBULANCE: return "Ambulance";
        case VEHICLE_POLICE:    return "Police";
        case VEHICLE_FIRE:      return "Fire Truck";
        default:                return "Unknown";
    }
}

/*
// Return a descriptive string for a FacilityType enum value
static inline const char *facility_type_str(FacilityType t) {
    switch (t) {
        case FACILITY_HOSPITAL:       return "Hospital";
        case FACILITY_POLICE_STATION: return "Police Station";
        case FACILITY_FIRE_STATION:   return "Fire Station";
        default:                      return "Unknown";
    }
}

// Return a descriptive string for an EmergencyType enum value
static inline const char *emergency_type_str(EmergencyType t) {
    switch (t) {
        case EMERGENCY_MEDICAL: return "Medical";
        case EMERGENCY_CRIME:   return "Crime";
        case EMERGENCY_FIRE:    return "Fire";
        default:                return "Unknown";
    }
}
*/

/* Return a descriptive string for a VehicleStatus enum value */
static inline const char *vehicle_status_str(VehicleStatus s) {
    switch (s) {
        case STATUS_AVAILABLE: return "Available";
        case STATUS_BUSY:      return "Busy";
        default:               return "Unknown";
    }
}

/*
// Map EmergencyType → matching VehicleType
static inline VehicleType emergency_to_vehicle_type(EmergencyType e) {
    switch (e) {
        case EMERGENCY_MEDICAL: return VEHICLE_AMBULANCE;
        case EMERGENCY_CRIME:   return VEHICLE_POLICE;
        case EMERGENCY_FIRE:    return VEHICLE_FIRE;
        default:                return VEHICLE_AMBULANCE;
    }
}

// Map EmergencyType → matching FacilityType
static inline FacilityType emergency_to_facility_type(EmergencyType e) {
    switch (e) {
        case EMERGENCY_MEDICAL: return FACILITY_HOSPITAL;
        case EMERGENCY_CRIME:   return FACILITY_POLICE_STATION;
        case EMERGENCY_FIRE:    return FACILITY_FIRE_STATION;
        default:                return FACILITY_HOSPITAL;
    }
}
*/

#endif /* COMMON_H */
