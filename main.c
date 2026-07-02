// main.c — Travel Planner (AVL Tree Only Edition)
//
// Data Structures:
//   Trip Database   : AVL Tree (KEY_DATE) — stores TripActivity
//                     in-order traversal = chronological order
//   Navigation DB   : AVL Tree (KEY_INT) per activity — stores NavStep
//                     in-order traversal = step sequence
//
// Both trees use the same AVLTree / AVLNode implementation (avl.c).
// The key type (KEY_DATE or KEY_INT) is set at tree creation time.
//
// Persistence: CSV file (data.csv) — same format as original project.

#include "menu.h"
#include "persistence.h"
#include "utils.h"

int main(void) {
    print_header("TRAVEL PLANNER [AVL Only] — Loading...");
    persistence_repair(CSV_PATH);
    g_db  = persistence_load(CSV_PATH);
    g_csv = CSV_PATH;
    if (g_db->count > 0)
        printf("  Loaded %d trip(s) from %s\n", g_db->count, CSV_PATH);
    else
        printf("  No trips found. Starting fresh.\n");
    main_loop();
    persistence_save(g_csv, g_db);
    trip_db_destroy(g_db);
    printf("\n  Saved to %s. Goodbye!\n\n", CSV_PATH);
    return 0;
}
