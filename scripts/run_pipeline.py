import requests
import json
import math
import random
import os

# Configuration
LAT = 18.5204
LON = 73.8567
RADIUS = 3000
OVERPASS_URL = "https://overpass-api.de/api/interpreter"

# Data output paths
DATA_DIR = os.path.join(os.path.dirname(os.path.dirname(__file__)), "data")
GRAPH_FILE = os.path.join(DATA_DIR, "city_map.txt")
FACILITIES_FILE = os.path.join(DATA_DIR, "facilities.txt")
VEHICLES_FILE = os.path.join(DATA_DIR, "vehicles.txt")

# Limits to match our C capacity (we'll set C to 3000 nodes just in case)
MAX_NODES = 3000 

def haversine(lat1, lon1, lat2, lon2):
    R = 6371.0 # Earth radius in km
    dlat = math.radians(lat2 - lat1)
    dlon = math.radians(lon2 - lon1)
    a = math.sin(dlat / 2)**2 + math.cos(math.radians(lat1)) * math.cos(math.radians(lat2)) * math.sin(dlon / 2)**2
    c = 2 * math.atan2(math.sqrt(a), math.sqrt(1 - a))
    return R * c * 1000 # distance in meters

def fetch_data():
    print("Fetching roads from Overpass API (this may take a moment)...")
    # We only fetch major roads to keep node count reasonable
    road_query = f"""[out:json];
(
  way["highway"~"^(primary|secondary|tertiary|residential)$"](around:{RADIUS},{LAT},{LON});
);
(._;>;);
out;"""
    headers = {'User-Agent': 'EmergencyRoutingSystem/1.0'}
    resp_roads = requests.post(OVERPASS_URL, data={'data': road_query}, headers=headers)
    if resp_roads.status_code != 200:
        print(f"Error fetching roads: {resp_roads.status_code}")
        print(resp_roads.text)
    roads_data = resp_roads.json()

    print("Fetching facilities from Overpass API...")
    fac_query = f"""[out:json];
(
  node["amenity"="hospital"](around:{RADIUS},{LAT},{LON});
  way["amenity"="hospital"](around:{RADIUS},{LAT},{LON});
  node["amenity"="fire_station"](around:{RADIUS},{LAT},{LON});
  way["amenity"="fire_station"](around:{RADIUS},{LAT},{LON});
  node["amenity"="police"](around:{RADIUS},{LAT},{LON});
  way["amenity"="police"](around:{RADIUS},{LAT},{LON});
);
out center;"""
    resp_fac = requests.post(OVERPASS_URL, data={'data': fac_query}, headers=headers)
    if resp_fac.status_code != 200:
        print(f"Error fetching facilities: {resp_fac.status_code}")
        print(resp_fac.text)
    fac_data = resp_fac.json()

    return roads_data, fac_data

def process_and_export(roads_data, fac_data):
    print("Processing graph data...")
    # 1. Parse Nodes
    way_nodes = set()
    node_to_name = {}
    for element in roads_data['elements']:
        if element['type'] == 'way' and 'nodes' in element:
            street_name = element.get('tags', {}).get('name', '')
            for nd in element['nodes']:
                way_nodes.add(nd)
                if street_name and nd not in node_to_name:
                    node_to_name[nd] = street_name

    raw_nodes = {}
    for element in roads_data['elements']:
        if element['type'] == 'node' and element['id'] in way_nodes:
            raw_nodes[element['id']] = (element['lat'], element['lon'])

    # 2. Parse Edges and keep only connected nodes
    temp_edges = []
    used_raw_nodes = set()
    for element in roads_data['elements']:
        if element['type'] == 'way' and 'nodes' in element:
            way_nds = element['nodes']
            for i in range(len(way_nds) - 1):
                u_raw = way_nds[i]
                v_raw = way_nds[i+1]
                if u_raw in raw_nodes and v_raw in raw_nodes:
                    used_raw_nodes.add(u_raw)
                    used_raw_nodes.add(v_raw)
                    temp_edges.append((u_raw, v_raw))

    # 3. Build Adjacency List from raw edges to find a connected subgraph
    raw_adj = {}
    for u, v in temp_edges:
        if u not in raw_adj: raw_adj[u] = []
        if v not in raw_adj: raw_adj[v] = []
        raw_adj[u].append(v)
        raw_adj[v].append(u)

    # Pick a starting node (ideally near the center)
    start_node = None
    min_dist = float('inf')
    for nid, (nlat, nlon) in raw_nodes.items():
        if nid in raw_adj:
            d = haversine(LAT, LON, nlat, nlon)
            if d < min_dist:
                min_dist = d
                start_node = nid

    # BFS to collect up to MAX_NODES connected nodes
    connected_raw_nodes = []
    if start_node:
        queue = [start_node]
        visited = {start_node}
        while queue and len(connected_raw_nodes) < MAX_NODES:
            curr = queue.pop(0)
            connected_raw_nodes.append(curr)
            for nbr in raw_adj[curr]:
                if nbr not in visited:
                    visited.add(nbr)
                    queue.append(nbr)
    
    # Create ID Mapping
    node_id_map = {}
    nodes_out = []
    current_node_id = 0
    
    for raw_id in connected_raw_nodes:
        node_id_map[raw_id] = current_node_id
        lat, lon = raw_nodes[raw_id]
        
        # Get street name if available
        name = node_to_name.get(raw_id, f"Street_{current_node_id}")
        name = name.replace(" ", "_").replace("'", "")[:30]
        name = "".join(c for c in name if c.isascii())
        if not name: name = f"Street_{current_node_id}"
        
        # We need to map real lat/lon to our 0-100 coordinate system for the QuadTree
        # We'll just use a simple normalization based on the bounding box
        nodes_out.append((current_node_id, lat, lon, name))
        current_node_id += 1

    # Normalize coordinates to 0-100 for QuadTree compatibility
    min_lat = min(n[1] for n in nodes_out)
    max_lat = max(n[1] for n in nodes_out)
    min_lon = min(n[2] for n in nodes_out)
    max_lon = max(n[2] for n in nodes_out)

    def normalize(lat, lon):
        x = ((lon - min_lon) / (max_lon - min_lon)) * 100.0 if max_lon != min_lon else 50.0
        y = ((lat - min_lat) / (max_lat - min_lat)) * 100.0 if max_lat != min_lat else 50.0
        x = max(0.0, min(100.0, x))
        y = max(0.0, min(100.0, y))
        return round(x, 2), round(y, 2)

    # 4. Filter and process edges
    edges_out = []
    adj_list = {}
    for u_raw, v_raw in temp_edges:
        if u_raw in node_id_map and v_raw in node_id_map:
            u = node_id_map[u_raw]
            v = node_id_map[v_raw]
            lat1, lon1 = raw_nodes[u_raw]
            lat2, lon2 = raw_nodes[v_raw]
            dist = haversine(lat1, lon1, lat2, lon2)
            
            # Simulate traffic on 15% of roads
            if random.random() < 0.15:
                dist *= random.choice([3.0, 4.0])
                
            edges_out.append((u, v, round(dist, 2)))
            edges_out.append((v, u, round(dist, 2))) # Bidirectional for simplicity
            
            if u not in adj_list: adj_list[u] = []
            if v not in adj_list: adj_list[v] = []
            adj_list[u].append(v)
            adj_list[v].append(u)

    print(f"Graph generated with {len(nodes_out)} nodes and {len(edges_out)//2} edges.")

    # Export Graph
    print(f"Exporting Graph ({len(nodes_out)} nodes, {len(edges_out)} edges) to {GRAPH_FILE}...")
    with open(GRAPH_FILE, 'w', encoding='utf-8') as f:
        f.write(f"NODES {len(nodes_out)}\n")
        for nid, lat, lon, name in nodes_out:
            nx, ny = normalize(lat, lon)
            f.write(f"{nid} {name} {nx} {ny}\n")
            
        f.write(f"EDGES {len(edges_out)}\n")
        for u, v, weight in edges_out:
            f.write(f"{u} {v} {weight}\n")

    # 5. Process Facilities
    print("Processing Facilities...")
    facilities = []
    fac_id_counter = 1
    
    # Types: 0=Medical, 1=Police, 2=Fire
    for element in fac_data['elements']:
        amenity = element.get('tags', {}).get('amenity', '')
        fac_type = 0
        if amenity == 'hospital': fac_type = 0
        elif amenity == 'police': fac_type = 1
        elif amenity == 'fire_station': fac_type = 2
        else: continue
            
        name = element.get('tags', {}).get('name', f"Facility_{fac_id_counter}")
        name = name.replace(" ", "_").replace("'", "")[:30] # Clean name
        # Remove non-ascii characters to be safe for C string parsing
        name = "".join(c for c in name if c.isascii())
        if not name: name = f"Facility_{fac_id_counter}"
        
        # Get location
        lat = element.get('lat') or element.get('center', {}).get('lat')
        lon = element.get('lon') or element.get('center', {}).get('lon')
        if not lat or not lon: continue
            
        # Snap to closest node
        closest_node = None
        min_d = float('inf')
        for nid, nlat, nlon, nname in nodes_out:
            d = haversine(lat, lon, nlat, nlon)
            if d < min_d:
                min_d = d
                closest_node = nid
                
        if closest_node is not None:
            nx, ny = normalize(lat, lon)
            capacity = random.randint(10, 100) if fac_type != 0 else random.randint(50, 500)
            facilities.append((fac_id_counter, fac_type, closest_node, nx, ny, capacity, name))
            fac_id_counter += 1

    print(f"Exporting {len(facilities)} Facilities to {FACILITIES_FILE}...")
    with open(FACILITIES_FILE, 'w', encoding='utf-8') as f:
        f.write(f"FACILITIES {len(facilities)}\n")
        for fac in facilities:
            f.write(f"{fac[0]} {fac[1]} {fac[2]} {fac[3]} {fac[4]} {fac[5]} {fac[6]}\n")

    # 6. Spawn Vehicles
    print("Processing Vehicles...")
    vehicles = []
    veh_id_counter = 1
    
    # Spawn at facilities
    for fac in facilities:
        fac_id, fac_type, node_id, nx, ny, cap, name = fac
        v_type = fac_type # Vehicle type matches facility type
        prefix = "Ambulance" if v_type == 0 else "Police" if v_type == 1 else "FireTruck"
        
        # 2 vehicles per facility
        for _ in range(2):
            v_name = f"{prefix}_{veh_id_counter}"
            vehicles.append((veh_id_counter, v_type, node_id, nx, ny, v_name))
            veh_id_counter += 1
            
    # Spawn patrolling vehicles randomly
    num_patrols = 15
    for _ in range(num_patrols):
        node = random.choice(nodes_out)
        nid, nlat, nlon, nname = node
        nx, ny = normalize(nlat, nlon)
        v_type = random.choice([0, 1, 2])
        prefix = "Patrol_Amb" if v_type == 0 else "Patrol_Pol" if v_type == 1 else "Patrol_Fire"
        v_name = f"{prefix}_{veh_id_counter}"
        vehicles.append((veh_id_counter, v_type, nid, nx, ny, v_name))
        veh_id_counter += 1

    print(f"Exporting {len(vehicles)} Vehicles to {VEHICLES_FILE}...")
    with open(VEHICLES_FILE, 'w', encoding='utf-8') as f:
        f.write(f"VEHICLES {len(vehicles)}\n")
        for veh in vehicles:
            f.write(f"{veh[0]} {veh[1]} {veh[2]} {veh[3]} {veh[4]} {veh[5]}\n")
            
    print("Pipeline completed successfully!")

if __name__ == "__main__":
    r_data, f_data = fetch_data()
    process_and_export(r_data, f_data)
