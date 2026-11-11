/*
 * ============================================================================
 *  Emergency Response Dispatch & Optimal Routing System
 *  main.c — Program entry point
 * ============================================================================
 */

#include "common.h"
#include "graph.h"
#include "dijkstra.h"
#include "avl_tree.h"
#include "trie.h"

/* ── Global data structures ───────────────────────────────────────────── */
static Graph   cityMap;
static AVLTree vehicleDB;
static Trie    locationDB;

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
    printf("  6.  View Pending Calls\n");
    printf("  7.  View City Map\n");
    printf("  8.  Find Shortest Path\n");
    printf("  9.  Complete a Dispatch\n");
    printf("  10. Exit\n");
    printf("========================================================\n");
    printf("  Enter choice: ");
}

/* ── Handle: View City Map ────────────────────────────────────────────── */
static void handle_view_map(void) {
    graph_display(&cityMap);
}

/* ── Handle: Find Shortest Path ───────────────────────────────────────── */
static void handle_shortest_path(void) {
    printf("\n--- Find Shortest Path ---\n");
    printf("  Available locations:\n");
    for (int i = 0; i < cityMap.nodeCount; i++) {
        printf("    [%2d] %s\n", i, cityMap.nodes[i].name);
    }

    int src, dest;
    printf("  Enter source node ID      : ");
    if (scanf("%d", &src) != 1) { while (getchar() != '\n'); return; }
    printf("  Enter destination node ID  : ");
    if (scanf("%d", &dest) != 1) { while (getchar() != '\n'); return; }

    DijkstraResult result;
    if (dijkstra_shortest_path(&cityMap, src, dest, &result) == 0) {
        dijkstra_print_result(&cityMap, &result, src, dest);
    }
}

/* ── Handle: View All Vehicles ─────────────────────────────────────────── */
static void handle_view_vehicles(void) {
    avl_inorder(&vehicleDB);
}

/* ── Handle: Search Location (Trie Autocomplete) ──────────────────────── */
static void handle_search_location(void) {
    char prefix[MAX_NAME_LEN];
    printf("\n--- Search Location (Autocomplete) ---\n");
    printf("  Enter location prefix: ");
    if (scanf("%63s", prefix) != 1) { while (getchar() != '\n'); return; }

    TrieMatch matches[10];
    int count = trie_search_prefix(&locationDB, prefix, matches, 10);

    if (count == 0) {
        printf("  [!] No locations found matching '%s'.\n", prefix);
    } else {
        printf("  Found %d matching location(s):\n", count);
        for (int i = 0; i < count; i++) {
            printf("    - %-15s (Node ID: %2d)\n", matches[i].name, matches[i].nodeID);
        }
    }
}

/* ── Main ─────────────────────────────────────────────────────────────── */

int main(void) {

    printf("============================================================\n");
    printf("  Emergency Response Dispatch & Optimal Routing System\n");
    printf("============================================================\n\n");

    /* ── Load predefined city map (10 Pune locations, with traffic) ── */
    graph_init(&cityMap);
    graph_load_predefined(&cityMap);

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

    /* ── Interactive menu loop ─────────────────────────────────────── */
    int choice;
    int running = 1;

    while (running) {
        print_menu();

        if (scanf("%d", &choice) != 1) {
            while (getchar() != '\n');
            printf("[!] Invalid input. Please enter a number.\n");
            continue;
        }

        switch (choice) {
            case 1:
            case 2:
            case 6:
            case 9:
                printf("[!] Option not needed for the final demo.\n");
                break;
            case 3:
                handle_search_location();
                break;
            case 4:
                handle_view_vehicles();
                break;
            case 5:
                printf("[!] Facilities (B+ Tree) not needed for final demo.\n");
                break;
            case 7:
                handle_view_map();
                break;
            case 8:
                handle_shortest_path();
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

    return 0;
}
