"""
============================================================================
 Emergency Response Dispatch & Optimal Routing System
 server.py — FastAPI + ctypes bridge to the C engine DLL
============================================================================

Run:
    pip install fastapi uvicorn
    python server.py

The server:
  1. Loads emergency_engine.dll via ctypes
  2. Initializes all 7 data structures from data/ files
  3. Serves the frontend/ directory as static files
  4. Exposes REST API endpoints for the frontend to fetch live data
"""

import os
import sys
import ctypes
from ctypes import (
    c_int, c_double, c_char, c_char_p,
    Structure, POINTER, byref, create_string_buffer
)
from pathlib import Path

from fastapi import FastAPI, HTTPException
from fastapi.staticfiles import StaticFiles
from fastapi.responses import FileResponse, JSONResponse
from fastapi.middleware.cors import CORSMiddleware
from pydantic import BaseModel
import uvicorn

# ═══════════════════════════════════════════════════════════════════════
#  ctypes struct definitions (must mirror api.h exactly)
# ═══════════════════════════════════════════════════════════════════════

class APITrieMatch(Structure):
    _fields_ = [
        ("nodeID", c_int),
        ("name", c_char * 64),
    ]

class APIVehicle(Structure):
    _fields_ = [
        ("id", c_int),
        ("type", c_int),
        ("locationNodeID", c_int),
        ("posX", c_double),
        ("posY", c_double),
        ("status", c_int),
        ("name", c_char * 64),
    ]

class APIFacility(Structure):
    _fields_ = [
        ("id", c_int),
        ("type", c_int),
        ("locationNodeID", c_int),
        ("posX", c_double),
        ("posY", c_double),
        ("capacity", c_int),
        ("name", c_char * 64),
    ]

class APIEmergencyCall(Structure):
    _fields_ = [
        ("callID", c_int),
        ("type", c_int),
        ("locationName", c_char * 64),
        ("locationNodeID", c_int),
        ("timestamp", c_int),
        ("priority", c_int),
    ]

class APIRouteStep(Structure):
    _fields_ = [
        ("nodeID", c_int),
        ("name", c_char * 64),
        ("posX", c_double),
        ("posY", c_double),
    ]

class APIDispatchResult(Structure):
    _fields_ = [
        ("callID", c_int),
        ("emergencyType", c_int),
        ("locationName", c_char * 64),
        ("destNodeID", c_int),
        ("vehicleID", c_int),
        ("vehicleName", c_char * 64),
        ("vehicleType", c_int),
        ("vehicleNodeID", c_int),
        ("facilityID", c_int),
        ("facilityName", c_char * 64),
        ("facilityType", c_int),
        ("facilityNodeID", c_int),
        ("facilityCapacity", c_int),
        ("vehicleRouteDistance", c_double),
        ("vehicleRoutePath", c_int * 500),
        ("vehicleRouteLength", c_int),
        ("facilityRouteDistance", c_double),
        ("facilityRoutePath", c_int * 500),
        ("facilityRouteLength", c_int),
        ("success", c_int),
        ("error", c_char * 128),
    ]

# ═══════════════════════════════════════════════════════════════════════
#  Load the DLL and set up function signatures
# ═══════════════════════════════════════════════════════════════════════

BASE_DIR = Path(__file__).parent
DLL_PATH = BASE_DIR / "emergency_engine.dll"
DATA_DIR = BASE_DIR / "data"

if not DLL_PATH.exists():
    print(f"[ERROR] DLL not found at {DLL_PATH}")
    print("        Run 'make dll' first to compile the engine.")
    sys.exit(1)

engine = ctypes.CDLL(str(DLL_PATH))

# Define function signatures
engine.api_init.argtypes = [c_char_p]
engine.api_init.restype = c_int

engine.api_shutdown.argtypes = []
engine.api_shutdown.restype = None

engine.api_search_prefix.argtypes = [c_char_p, POINTER(APITrieMatch), c_int]
engine.api_search_prefix.restype = c_int

engine.api_search_exact.argtypes = [c_char_p]
engine.api_search_exact.restype = c_int

engine.api_add_emergency.argtypes = [c_int, c_char_p, c_int]
engine.api_add_emergency.restype = c_int

engine.api_get_queue.argtypes = [POINTER(APIEmergencyCall), c_int]
engine.api_get_queue.restype = c_int

engine.api_get_queue_count.argtypes = []
engine.api_get_queue_count.restype = c_int

engine.api_dispatch.argtypes = []
engine.api_dispatch.restype = APIDispatchResult

engine.api_get_vehicles.argtypes = [POINTER(APIVehicle), c_int]
engine.api_get_vehicles.restype = c_int

engine.api_get_vehicle_count.argtypes = []
engine.api_get_vehicle_count.restype = c_int

engine.api_resolve_vehicle.argtypes = [c_int]
engine.api_resolve_vehicle.restype = c_int

engine.api_get_facilities.argtypes = [POINTER(APIFacility), c_int]
engine.api_get_facilities.restype = c_int

engine.api_get_facility_count.argtypes = []
engine.api_get_facility_count.restype = c_int

engine.api_get_route.argtypes = [c_int, c_int, POINTER(c_int), c_int, POINTER(c_double)]
engine.api_get_route.restype = c_int

engine.api_get_node_info.argtypes = [c_int]
engine.api_get_node_info.restype = APIRouteStep

engine.api_get_node_count.argtypes = []
engine.api_get_node_count.restype = c_int

engine.api_get_edge_count.argtypes = []
engine.api_get_edge_count.restype = c_int

# ═══════════════════════════════════════════════════════════════════════
#  Initialize the C engine
# ═══════════════════════════════════════════════════════════════════════

print(f"[Server] Loading engine from {DLL_PATH}")
print(f"[Server] Data directory: {DATA_DIR}")

result = engine.api_init(str(DATA_DIR).encode("utf-8"))
if result != 0:
    print("[ERROR] Failed to initialize engine. Check data files.")
    sys.exit(1)

print(f"[Server] Engine ready: {engine.api_get_node_count()} nodes, "
      f"{engine.api_get_vehicle_count()} vehicles, "
      f"{engine.api_get_facility_count()} facilities")

# ═══════════════════════════════════════════════════════════════════════
#  FastAPI Application
# ═══════════════════════════════════════════════════════════════════════

app = FastAPI(
    title="CitizenConnect Emergency Response API",
    description="REST API bridging Python to the C dispatch engine via ctypes",
    version="1.0.0",
)

app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_methods=["*"],
    allow_headers=["*"],
)

# ── Pydantic models for request bodies ──────────────────────────────

class EmergencyRequest(BaseModel):
    type: int           # 0=Medical, 1=Crime, 2=Fire
    locationName: str
    priority: int       # 1=Critical, 2=High, 3=Normal

# ── Helper to decode ctypes char arrays ─────────────────────────────

VEHICLE_TYPES = {0: "Ambulance", 1: "Police", 2: "Fire Truck"}
FACILITY_TYPES = {0: "Hospital", 1: "Police Station", 2: "Fire Station"}
EMERGENCY_TYPES = {0: "Medical", 1: "Crime", 2: "Fire"}
VEHICLE_STATUS = {0: "Available", 1: "Busy"}

def decode(b):
    """Decode a ctypes char array to Python string."""
    if isinstance(b, bytes):
        return b.decode("utf-8", errors="replace").rstrip("\x00")
    return str(b)

# ═══════════════════════════════════════════════════════════════════════
#  API Endpoints
# ═══════════════════════════════════════════════════════════════════════

active_missions = []

@app.get("/api/search")
def search_locations(prefix: str, limit: int = 20):
    """Trie prefix search for location autocomplete."""
    results = (APITrieMatch * limit)()
    count = engine.api_search_prefix(prefix.encode("utf-8"), results, limit)
    return [
        {"nodeID": results[i].nodeID, "name": decode(results[i].name)}
        for i in range(count)
    ]

@app.post("/api/emergency")
def add_emergency(req: EmergencyRequest):
    """Add an emergency to the priority queue."""
    callID = engine.api_add_emergency(
        req.type,
        req.locationName.encode("utf-8"),
        req.priority
    )
    if callID < 0:
        raise HTTPException(status_code=500, detail="Failed to enqueue emergency")
    return {
        "callID": callID,
        "type": EMERGENCY_TYPES.get(req.type, "Unknown"),
        "locationName": req.locationName,
        "priority": req.priority,
        "status": "queued"
    }

@app.get("/api/queue")
def get_queue():
    """Get all emergencies currently in the priority queue."""
    max_items = 100
    results = (APIEmergencyCall * max_items)()
    count = engine.api_get_queue(results, max_items)
    return {
        "count": count,
        "items": [
            {
                "callID": results[i].callID,
                "type": EMERGENCY_TYPES.get(results[i].type, "Unknown"),
                "typeCode": results[i].type,
                "locationName": decode(results[i].locationName),
                "locationNodeID": results[i].locationNodeID,
                "timestamp": results[i].timestamp,
                "priority": results[i].priority,
            }
            for i in range(count)
        ]
    }

@app.post("/api/dispatch")
def dispatch():
    """Process the next emergency: dequeue → QuadTree → Dijkstra → AVL update."""
    res = engine.api_dispatch()
    if not res.success:
        error_msg = decode(res.error)
        raise HTTPException(status_code=404, detail=error_msg)

    # Build vehicle route with node names
    vehicle_route = []
    for i in range(res.vehicleRouteLength):
        nid = res.vehicleRoutePath[i]
        info = engine.api_get_node_info(nid)
        vehicle_route.append({
            "nodeID": nid,
            "name": decode(info.name),
            "posX": info.posX,
            "posY": info.posY,
        })

    # Build facility route with node names
    facility_route = []
    for i in range(res.facilityRouteLength):
        nid = res.facilityRoutePath[i]
        info = engine.api_get_node_info(nid)
        facility_route.append({
            "nodeID": nid,
            "name": decode(info.name),
            "posX": info.posX,
            "posY": info.posY,
        })

    res_dict = {
        "callID": res.callID,
        "emergencyType": EMERGENCY_TYPES.get(res.emergencyType, "Unknown"),
        "locationName": decode(res.locationName),
        "destNodeID": res.destNodeID,
        "vehicle": {
            "id": res.vehicleID,
            "name": decode(res.vehicleName),
            "type": VEHICLE_TYPES.get(res.vehicleType, "Unknown"),
            "fromNodeID": res.vehicleNodeID,
            "routeDistance": round(res.vehicleRouteDistance, 2),
            "route": vehicle_route,
        },
        "facility": {
            "id": res.facilityID,
            "name": decode(res.facilityName),
            "type": FACILITY_TYPES.get(res.facilityType, "Unknown"),
            "nodeID": res.facilityNodeID,
            "capacity": res.facilityCapacity,
            "routeDistance": round(res.facilityRouteDistance, 2),
            "route": facility_route,
        } if res.facilityID > 0 else None,
        "status": "dispatched"
    }
    
    active_missions.append(res_dict)
    return res_dict

@app.get("/api/active_mission")
def get_active_mission():
    """Get the most recently dispatched mission for the operator console."""
    if not active_missions:
        return {"has_mission": False}
    return {"has_mission": True, "mission": active_missions[-1]}

@app.get("/api/vehicles")
def get_vehicles():
    """Get all vehicles from the AVL tree (in-order traversal)."""
    max_items = 500
    results = (APIVehicle * max_items)()
    count = engine.api_get_vehicles(results, max_items)
    return {
        "count": count,
        "items": [
            {
                "id": results[i].id,
                "name": decode(results[i].name),
                "type": VEHICLE_TYPES.get(results[i].type, "Unknown"),
                "typeCode": results[i].type,
                "locationNodeID": results[i].locationNodeID,
                "posX": round(results[i].posX, 4),
                "posY": round(results[i].posY, 4),
                "status": VEHICLE_STATUS.get(results[i].status, "Unknown"),
                "statusCode": results[i].status,
            }
            for i in range(count)
        ]
    }

@app.post("/api/resolve/{vehicle_id}")
def resolve_vehicle(vehicle_id: int):
    """Mark a vehicle as available (resolve emergency)."""
    result = engine.api_resolve_vehicle(vehicle_id)
    if result != 0:
        raise HTTPException(status_code=404, detail=f"Vehicle {vehicle_id} not found")
        
    # Remove from active_missions
    global active_missions
    active_missions = [m for m in active_missions if m["vehicle"]["id"] != vehicle_id]
    
    return {"vehicleID": vehicle_id, "status": "Available", "message": "Vehicle freed"}

@app.get("/api/facilities")
def get_facilities():
    """Get all facilities from the B+ tree."""
    max_items = 300
    results = (APIFacility * max_items)()
    count = engine.api_get_facilities(results, max_items)
    return {
        "count": count,
        "items": [
            {
                "id": results[i].id,
                "name": decode(results[i].name),
                "type": FACILITY_TYPES.get(results[i].type, "Unknown"),
                "typeCode": results[i].type,
                "locationNodeID": results[i].locationNodeID,
                "posX": round(results[i].posX, 4),
                "posY": round(results[i].posY, 4),
                "capacity": results[i].capacity,
            }
            for i in range(count)
        ]
    }

@app.get("/api/route")
def get_route(src: int, dest: int):
    """Compute shortest path between two nodes using Dijkstra + Fibonacci Heap."""
    path = (c_int * 500)()
    distance = c_double(0)
    length = engine.api_get_route(src, dest, path, 500, byref(distance))
    if length < 0:
        raise HTTPException(status_code=404, detail="No route found")

    route_steps = []
    for i in range(length):
        nid = path[i]
        info = engine.api_get_node_info(nid)
        route_steps.append({
            "nodeID": nid,
            "name": decode(info.name),
            "posX": info.posX,
            "posY": info.posY,
        })

    return {
        "source": src,
        "destination": dest,
        "distance": round(distance.value, 2),
        "pathLength": length,
        "route": route_steps,
    }

@app.get("/api/stats")
def get_stats():
    """Get system-wide statistics."""
    return {
        "nodes": engine.api_get_node_count(),
        "edges": engine.api_get_edge_count(),
        "vehicles": engine.api_get_vehicle_count(),
        "facilities": engine.api_get_facility_count(),
        "queueSize": engine.api_get_queue_count(),
    }

# ═══════════════════════════════════════════════════════════════════════
#  Serve the frontend
# ═══════════════════════════════════════════════════════════════════════

FRONTEND_DIR = BASE_DIR / "frontend"

@app.get("/")
def serve_index():
    return FileResponse(FRONTEND_DIR / "index.html")

# Mount frontend as static files (for CSS, JS, and other HTML pages)
app.mount("/", StaticFiles(directory=str(FRONTEND_DIR), html=True), name="frontend")

# ═══════════════════════════════════════════════════════════════════════
#  Entry point
# ═══════════════════════════════════════════════════════════════════════

if __name__ == "__main__":
    print("\n" + "=" * 60)
    print("  CitizenConnect — Emergency Response API Server")
    print("  http://localhost:8000")
    print("  API docs: http://localhost:8000/docs")
    print("=" * 60 + "\n")
    uvicorn.run(app, host="0.0.0.0", port=8000)
