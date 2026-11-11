/*
 * ============================================================================
 *  Emergency Response Dispatch & Optimal Routing System
 *  main.c — Program entry point
 * ============================================================================
 */

#include "avl_tree.h"
#include "bplus_tree.h"
#include "common.h"
#include "dijkstra.h"
#include "graph.h"
#include "quadtree.h"
#include "queue.h"
#include "trie.h"

/* ── Global data structures ───────────────────────────────────────────── */
static Graph cityMap;
static AVLTree vehicleDB;
static Trie locationDB;
static QuadTree spatialDB;
static BPTree facilityDB;
static EmergencyQueue callQueue;

/* ── Utility: print the main menu ─────────────────────────────────────── */
static void print_menu(void) {
  printf("\n");
  printf("========================================================\n");
  printf("   Emergency Response Dispatch & Optimal Routing System\n");
  printf("========================================================\n");
  printf("  1.  Add Emergency Call\n");
  printf("  2.  Process Next Emergency\n");
  printf("  3.  Search Location\n");
  printf("  4.  View All Vehicles\n");
  printf("  5.  View All Facilities\n");
  printf("  6.  View City Map\n");
  printf("  7.  Find Shortest Path\n");
  printf("  8.  Complete a Dispatch\n");
  printf("  9.  Resolve Emergency (Free Vehicle)\n");
  printf("  10. Exit\n");
  printf("========================================================\n");
  printf("  Enter choice: ");
}

/* ── Handle: Add Emergency ────────────────────────────────────────────── */
static void handle_add_emergency(void) {
  int eType, priority;
  char location[MAX_NAME_LEN];
  static int virtual_clock = 1000;

  printf("\n--- Add Emergency Call ---\n");
  printf("  Enter Emergency Type (0=Medical, 1=Crime, 2=Fire): ");
  if (scanf("%d", &eType) != 1) {
    while (getchar() != '\n')
      ;
    return;
  }

  printf("  Enter Priority (1=Critical, 2=High, 3=Normal): ");
  if (scanf("%d", &priority) != 1) {
    while (getchar() != '\n')
      ;
    return;
  }

  char prefix[MAX_NAME_LEN];
  printf("  Enter Location Prefix: ");
  if (scanf("%63s", prefix) != 1) {
    while (getchar() != '\n')
      ;
    return;
  }

  TrieMatch matches[10];
  int count = trie_search_prefix(&locationDB, prefix, matches, 10);

  if (count == 0) {
    printf("  [!] No locations found matching '%s'.\n", prefix);
    return;
  } else if (count == 1) {
    printf("  [+] Auto-selected location: %s\n", matches[0].name);
    strncpy(location, matches[0].name, MAX_NAME_LEN - 1);
    location[MAX_NAME_LEN - 1] = '\0';
  } else {
    printf("  Found %d matching location(s):\n", count);
    for (int i = 0; i < count; i++) {
      printf("    [%d] %s\n", i, matches[i].name);
    }
    int selection;
    printf("  Select location index (0-%d): ", count - 1);
    if (scanf("%d", &selection) != 1 || selection < 0 || selection >= count) {
      printf("  [!] Invalid selection.\n");
      while (getchar() != '\n')
        ;
      return;
    }
    strncpy(location, matches[selection].name, MAX_NAME_LEN - 1);
    location[MAX_NAME_LEN - 1] = '\0';
  }

  if (queue_enqueue(&callQueue, (EmergencyType)eType, location, priority,
                    virtual_clock++) == 0) {
    printf("  [+] Emergency queued successfully!\n");
  }
  queue_display(&callQueue);
}

/* ── Handle: Process Next Emergency ───────────────────────────────────── */
static void handle_process_next(void) {
  printf("\n--- Process Next Emergency ---\n");
  EmergencyCall call;
  if (queue_dequeue(&callQueue, &call)) {
    printf("  Popped Call %d: %s at '%s' (Priority %d)\n", call.callID,
           emergency_type_str(call.type), call.locationName, call.priority);
    // printf("  ... (Dispatch mapping logic pending Phase 9)\n");
  } else {
    printf("  [!] Queue is empty.\n");
  }
}

/* ── Handle: Complete a Dispatch (Phase 9) ────────────────────────────── */
static void handle_complete_dispatch(void) {
  printf("\n--- Complete a Dispatch ---\n");
  EmergencyCall call;
  if (!queue_dequeue(&callQueue, &call)) {
    printf("  [!] Queue is empty. No emergencies to dispatch.\n");
    return;
  }

  printf("  Dispatching Call %d: %s at '%s' (Priority %d)\n", call.callID,
         emergency_type_str(call.type), call.locationName, call.priority);

  int destNode = trie_search_exact(&locationDB, call.locationName);
  if (destNode == -1) {
    printf("  [!] Location '%s' not found in city map.\n", call.locationName);
    return;
  }

  double x = cityMap.nodes[destNode].position.x;
  double y = cityMap.nodes[destNode].position.y;

  VehicleType vType = emergency_to_vehicle_type(call.type);
  FacilityType fType = emergency_to_facility_type(call.type);

  QTPoint nearestVeh, nearestFac;
  int vFound = quadtree_query_nearest_by_type(
      &spatialDB, x, y, QT_ENTITY_VEHICLE, (int)vType, &nearestVeh);
  int fFound = quadtree_query_nearest_by_type(
      &spatialDB, x, y, QT_ENTITY_FACILITY, (int)fType, &nearestFac);

  Vehicle *v = NULL;
  if (vFound == 0) {
    v = avl_search(&vehicleDB, nearestVeh.entityID);
  }

  Facility *f = NULL;
  if (fFound == 0) {
    f = bptree_search(&facilityDB, nearestFac.entityID);
  }

  if (v) {
    printf("\n  [+] Dispatched %s (ID: %d) from '%s' (Node %d) to emergency at '%s' (Node %d).\n",
           v->name, v->id, cityMap.nodes[v->locationNodeID].name, v->locationNodeID,
           cityMap.nodes[destNode].name, destNode);
    DijkstraResult vRes;
    if (dijkstra_shortest_path(&cityMap, v->locationNodeID, destNode, &vRes) == 0) {
      dijkstra_print_result(&cityMap, &vRes, v->locationNodeID, destNode);
    }
    v->status = STATUS_BUSY;
  } else {
    printf("\n  [-] No available %s found.\n", vehicle_type_str(vType));
  }

  if (f) {
    printf("\n  [+] Routing emergency from '%s' (Node %d) to %s (ID: %d) at '%s' (Node %d).\n",
           cityMap.nodes[destNode].name, destNode, f->name, f->id,
           cityMap.nodes[f->locationNodeID].name, f->locationNodeID);
    DijkstraResult fRes;
    if (dijkstra_shortest_path(&cityMap, destNode, f->locationNodeID, &fRes) == 0) {
      dijkstra_print_result(&cityMap, &fRes, destNode, f->locationNodeID);
    }
  } else {
    printf("\n  [-] No available %s found.\n", facility_type_str(fType));
  }
}

/* ── Handle: View City Map ────────────────────────────────────────────── */
static void handle_view_map(void) { graph_display(&cityMap); }

/* ── Handle: Find Shortest Path ───────────────────────────────────────── */
static void handle_shortest_path(void) {
  printf("\n--- Find Shortest Path ---\n");
  printf("  Available locations:\n");
  for (int i = 0; i < cityMap.nodeCount; i++) {
    printf("    [%2d] %s\n", i, cityMap.nodes[i].name);
  }

  int src, dest;
  printf("  Enter source node ID      : ");
  if (scanf("%d", &src) != 1) {
    while (getchar() != '\n')
      ;
    return;
  }
  printf("  Enter destination node ID  : ");
  if (scanf("%d", &dest) != 1) {
    while (getchar() != '\n')
      ;
    return;
  }

  DijkstraResult result;
  if (dijkstra_shortest_path(&cityMap, src, dest, &result) == 0) {
    dijkstra_print_result(&cityMap, &result, src, dest);
  }
}

/* ── Handle: Resolve Emergency (Free Vehicle) ─────────────────────────── */
static void handle_resolve_emergency(void) {
  printf("\n--- Resolve Emergency (Free Vehicle) ---\n");
  int vid;
  printf("  Enter Vehicle ID to free: ");
  if (scanf("%d", &vid) != 1) {
    while (getchar() != '\n');
    return;
  }
  Vehicle *v = avl_search(&vehicleDB, vid);
  if (v) {
    v->status = STATUS_AVAILABLE;
    printf("  [+] Vehicle '%s' (ID: %d) is now AVAILABLE.\n", v->name, v->id);
  } else {
    printf("  [-] Vehicle ID %d not found.\n", vid);
  }
}

/* ── Handle: View All Vehicles ─────────────────────────────────────────── */
static void handle_view_vehicles(void) { avl_inorder(&vehicleDB); }

/* ── Handle: View All Facilities ───────────────────────────────────────── */
static void handle_view_facilities(void) { bptree_display(&facilityDB); }



/* ── Handle: Search Location (Trie Autocomplete) ──────────────────────── */
static void handle_search_location(void) {
  char prefix[MAX_NAME_LEN];
  printf("\n--- Search Location (Autocomplete) ---\n");
  printf("  Enter location prefix: ");
  if (scanf("%63s", prefix) != 1) {
    while (getchar() != '\n')
      ;
    return;
  }

  TrieMatch matches[10];
  int count = trie_search_prefix(&locationDB, prefix, matches, 10);

  if (count == 0) {
    printf("  [!] No locations found matching '%s'.\n", prefix);
  } else {
    printf("  Found %d matching location(s):\n", count);
    for (int i = 0; i < count; i++) {
      printf("    - %-15s (Node ID: %2d)\n", matches[i].name,
             matches[i].nodeID);
    }
  }
}

/* ── Main ─────────────────────────────────────────────────────────────── */

int main(void) {

  printf("============================================================\n");
  printf("  Emergency Response System\n");
  printf("============================================================\n\n");

  /* ── Load generated city map (Real-World OSM Data) ── */
  graph_init(&cityMap);
  if (graph_load_from_file(&cityMap, "data/city_map.txt") < 0) {
    fprintf(stderr, "[Main] Failed to load city map. Exiting.\n");
    return 1;
  }

  /* ── Phase 3: Load vehicles ───────────────────────────────────── */
  avl_init(&vehicleDB);
  if (avl_load_from_file(&vehicleDB, "data/vehicles.txt") < 0) {
    fprintf(stderr, "[Main] Failed to load vehicles. Exiting.\n");
    graph_free(&cityMap);
    return 1;
  }

  printf("\nSystem ready. %d locations, %d vehicles loaded.\n",
         cityMap.nodeCount, vehicleDB.count);

  /* ── Phase 5: Load Location Search Trie ───────────────────────── */
  trie_init(&locationDB);
  trie_load_from_graph(&locationDB, &cityMap);

  /* ── Phase 6 & Phase 7: Load Quad Tree & Facilities ───────────── */
  QTBoundary cityBounds = qt_boundary_create(
      CITY_WIDTH / 2.0, CITY_HEIGHT / 2.0, CITY_WIDTH / 2.0, CITY_HEIGHT / 2.0);
  quadtree_init(&spatialDB, cityBounds);
  quadtree_load_vehicles(&spatialDB, vehicleDB.root);

  bptree_init(&facilityDB);
  if (bptree_load_from_file(&facilityDB, "data/facilities.txt", &spatialDB) <
      0) {
    fprintf(stderr, "[Main] Failed to load facilities.\n");
  }

  /* ── Inject Facilities into Trie for Autocomplete ──────────────── */
  {
    Facility facs[MAX_FACILITIES];
    int count = bptree_range_query(&facilityDB, 0, 999999, facs, MAX_FACILITIES);
    for (int i = 0; i < count; i++) {
        trie_insert(&locationDB, facs[i].name, facs[i].locationNodeID);
    }
    printf("[Trie] Loaded %d facilities into autocomplete.\n", count);
  }

  /* ── Phase 8: Emergency Priority Queue ────────────────────────── */
  queue_init(&callQueue);

  /* ── Interactive menu loop ─────────────────────────────────────── */
  int choice;
  int running = 1;

  while (running) {
    print_menu();

    if (scanf("%d", &choice) != 1) {
      while (getchar() != '\n')
        ;
      printf("[!] Invalid input. Please enter a number.\n");
      continue;
    }

    switch (choice) {
    case 1:
      handle_add_emergency();
      break;
    case 2:
      handle_process_next();
      break;
    case 3:
      handle_search_location();
      break;
    case 4:
      handle_view_vehicles();
      break;
    case 5:
      handle_view_facilities();
      break;
    case 6:
      handle_view_map();
      break;
    case 7:
      handle_shortest_path();
      break;
    case 8:
      handle_complete_dispatch();
      break;
    case 9:
      handle_resolve_emergency();
      break;
    case 10:
      printf("Exiting. Stay safe!\n");
      running = 0;
      break;
    default:
      printf("[!] Invalid choice. Try again.\n");
      break;
    }
  }

  /* ── Cleanup ───────────────────────────────────────────────────── */
  graph_free(&cityMap);
  avl_free(&vehicleDB);
  trie_free(&locationDB);
  quadtree_free(&spatialDB);
  bptree_free(&facilityDB);
  queue_free(&callQueue);

  return 0;
}
