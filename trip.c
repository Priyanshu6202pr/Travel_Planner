#include "trip.h"
#include "utils.h"
#include <ctype.h>

/* ═══════════════════════════════════════════════════════════════════════
 * Utility
 * ═══════════════════════════════════════════════════════════════════════ */

const char *act_type_str(ActivityType t) {
    switch (t) {
        case ACT_FLIGHT:    return "FLIGHT";
        case ACT_HOTEL:     return "HOTEL";
        case ACT_TOURIST:   return "TOURIST";
        case ACT_TRANSPORT: return "TRANSPORT";
    }
    return "UNKNOWN";
}

ActivityType str_to_act_type(const char *s) {
    if (!strcmp(s,"FLIGHT"))    return ACT_FLIGHT;
    if (!strcmp(s,"HOTEL"))     return ACT_HOTEL;
    if (!strcmp(s,"TOURIST"))   return ACT_TOURIST;
    if (!strcmp(s,"TRANSPORT")) return ACT_TRANSPORT;
    return ACT_TRANSPORT;
}

/* ═══════════════════════════════════════════════════════════════════════
 * TripDB  — just a thin container for the trip AVL root pointer
 * ═══════════════════════════════════════════════════════════════════════ */

TripDB *trip_db_create(void) {
    TripDB *db = (TripDB *)calloc(1, sizeof(TripDB));
    if (!db) { fprintf(stderr,"[TRIP] OOM\n"); exit(1); }
    db->next_trip_id = 1;
    db->trip_root    = NULL;
    return db;
}

void trip_db_destroy(TripDB *db) {
    if (!db) return;
    trip_avl_free_all(db->trip_root);  /* recursively frees every TripRecord,
                                          TripActivity, and NavStep */
    free(db);
}

/* ── Create a new trip and insert it into the TripDB AVL tree ────────── */
TripRecord *trip_create(TripDB *db, const char *name,
                         const char *start_loc, const char *end_loc) {
    TripRecord *r = (TripRecord *)calloc(1, sizeof(TripRecord));
    if (!r) { fprintf(stderr,"[TRIP] OOM\n"); exit(1); }
    r->trip_id     = db->next_trip_id++;
    r->next_act_id = 1;
    r->act_root    = NULL;
    r->left = r->right = NULL;
    r->height = 1;
    strncpy(r->name,           name,      NAME_LEN - 1);
    strncpy(r->start_location, start_loc, LOC_LEN  - 1);
    strncpy(r->end_location,   end_loc,   LOC_LEN  - 1);
    db->trip_root = trip_avl_insert(db->trip_root, r);
    db->count++;
    return r;
}

/* ── Find a trip by ID  O(log n) ─────────────────────────────────────── */
TripRecord *trip_find(TripDB *db, int trip_id) {
    return trip_avl_search(db->trip_root, trip_id);
}

/* ── List all trips (in trip_id order) ───────────────────────────────── */
void trip_list(TripDB *db) {
    if (!db->trip_root) { printf("  (no trips)\n"); return; }
    printf("  %-6s  %-25s  %-20s  %-20s  %s\n",
           "TripID","Name","From","To","Activities");
    print_sep('-', 82);
    TripRecord *arr[MAX_NODES];
    int cnt = trip_avl_collect(db->trip_root, arr, MAX_NODES);
    for (int i = 0; i < cnt; i++) {
        TripRecord *r = arr[i];
        printf("  %-6d  %-25s  %-20s  %-20s  %d\n",
               r->trip_id, r->name,
               r->start_location[0] ? r->start_location : "-",
               r->end_location[0]   ? r->end_location   : "-",
               act_avl_size(r->act_root));
    }
}

/* ═══════════════════════════════════════════════════════════════════════
 * TripActivity  CRUD
 * ═══════════════════════════════════════════════════════════════════════ */

TripActivity *activity_make(int act_id, int trip_id, ActivityType type,
                             const char *location, const char *date,
                             ActivityDetails details) {
    TripActivity *a = (TripActivity *)calloc(1, sizeof(TripActivity));
    if (!a) { fprintf(stderr,"[TRIP] OOM\n"); exit(1); }
    a->act_id  = act_id;
    a->trip_id = trip_id;
    a->type    = type;
    a->nav_root = NULL;
    a->left = a->right = NULL;
    a->height = 1;
    strncpy(a->location, location, LOC_LEN  - 1);
    strncpy(a->date,     date,     DATE_LEN - 1);
    a->details = details;
    return a;
}

void activity_free(TripActivity *act) {
    if (!act) return;
    nav_avl_free_all(act->nav_root);
    free(act);
}

/* Insert into the activity AVL tree (auto-ordered by date) */
int trip_add_activity(TripRecord *trip, TripActivity *act) {
    int old_size = act_avl_size(trip->act_root);
    trip->act_root = act_avl_insert(trip->act_root, act);
    return (act_avl_size(trip->act_root) > old_size) ? 1 : 0;
}

/* Collect all activities in chronological order */
int trip_collect(TripRecord *trip, TripActivity **arr, int max) {
    return act_avl_collect(trip->act_root, arr, max);
}

/* Find by act_id (linear scan of in-order array) */
TripActivity *trip_find_activity(TripRecord *trip, int act_id) {
    TripActivity *arr[MAX_NODES];
    int cnt = trip_collect(trip, arr, MAX_NODES);
    for (int i = 0; i < cnt; i++)
        if (arr[i]->act_id == act_id) return arr[i];
    return NULL;
}

/* Renumber act_ids 1,2,3... in chronological order */
void trip_renumber(TripRecord *trip) {
    TripActivity *arr[MAX_NODES];
    int cnt = trip_collect(trip, arr, MAX_NODES);
    for (int i = 0; i < cnt; i++) arr[i]->act_id = i + 1;
    trip->next_act_id = cnt + 1;
}

/* Delete activity by act_id */
int trip_delete_activity(TripRecord *trip, int act_id) {
    TripActivity *target = trip_find_activity(trip, act_id);
    if (!target) return 0;
    char date_copy[DATE_LEN];
    strncpy(date_copy, target->date, DATE_LEN - 1);
    TripActivity *removed = NULL;
    trip->act_root = act_avl_delete(trip->act_root, date_copy, &removed);
    if (removed) {
        nav_avl_free_all(removed->nav_root);
        free(removed);
        trip_renumber(trip);
        return 1;
    }
    return 0;
}

/* Range search D1 to D2 */
int trip_range_search(TripRecord *trip, const char *d1, const char *d2,
                      TripActivity **out, int max) {
    return act_avl_range(trip->act_root, d1, d2, out, max);
}

/* ═══════════════════════════════════════════════════════════════════════
 * Display helpers
 * ═══════════════════════════════════════════════════════════════════════ */

static void print_act_detail(TripActivity *act) {
    switch (act->type) {
        case ACT_FLIGHT:
            printf("      Flight : %-12s  Airline: %s\n",
                   act->details.flight.flight_no, act->details.flight.airline);
            break;
        case ACT_HOTEL:
            printf("      Hotel  : %-30s  Charges: INR %.2f\n",
                   act->details.hotel.hotel_name, act->details.hotel.charges);
            break;
        case ACT_TOURIST:
            printf("      Place  : %s\n", act->details.tourist.place_name);
            break;
        case ACT_TRANSPORT:
            printf("      Mode   : %s\n", act->details.transport.mode);
            break;
    }
}

void trip_display(TripRecord *trip) {
    printf("\n"); print_sep('=', 65);
    printf("  Trip #%d : %s\n", trip->trip_id, trip->name);
    printf("  From    : %s\n", trip->start_location[0] ? trip->start_location : "(not set)");
    printf("  To      : %s\n", trip->end_location[0]   ? trip->end_location   : "(not set)");
    print_sep('=', 65);
    TripActivity *arr[MAX_NODES];
    int cnt = trip_collect(trip, arr, MAX_NODES);
    if (!cnt) { printf("  (no activities)\n\n"); return; }
    for (int i = 0; i < cnt; i++) {
        TripActivity *act = arr[i];
        printf("\n  [%d] ActID:%-3d  %-10s  %-25s  (%s)\n",
               i+1, act->act_id, act_type_str(act->type), act->location, act->date);
        print_act_detail(act);
        if (act->nav_root) nav_display(act);
    }
    printf("\n"); print_sep('-', 65);
    printf("  Total: %d activities\n\n", cnt);
}

void trip_display_summary(TripRecord *trip) {
    printf("\n  Trip #%d : %s  |  %s  ->  %s\n",
           trip->trip_id, trip->name,
           trip->start_location[0] ? trip->start_location : "?",
           trip->end_location[0]   ? trip->end_location   : "?");
    printf("  %-6s  %-10s  %-25s  %-18s  %s\n",
           "ActID","Type","Location","Date","Detail");
    print_sep('-', 80);
    TripActivity *arr[MAX_NODES];
    int cnt = trip_collect(trip, arr, MAX_NODES);
    for (int i = 0; i < cnt; i++) {
        TripActivity *act = arr[i];
        char det[80] = "-";
        switch (act->type) {
            case ACT_FLIGHT:
                snprintf(det,79,"%.14s / %.14s",
                         act->details.flight.flight_no, act->details.flight.airline); break;
            case ACT_HOTEL:
                snprintf(det,79,"%.20s INR %.0f",
                         act->details.hotel.hotel_name, act->details.hotel.charges); break;
            case ACT_TOURIST:
                snprintf(det,79,"%s", act->details.tourist.place_name); break;
            case ACT_TRANSPORT:
                snprintf(det,79,"%s", act->details.transport.mode); break;
        }
        printf("  %-6d  %-10s  %-25s  %-18s  %s\n",
               act->act_id, act_type_str(act->type),
               act->location, act->date, det);
    }
    printf("\n");
}

/* ═══════════════════════════════════════════════════════════════════════
 * Navigation operations
 * ═══════════════════════════════════════════════════════════════════════ */

NavStep *navstep_make(int step_id, const char *dir, float dist) {
    NavStep *ns = (NavStep *)calloc(1, sizeof(NavStep));
    if (!ns) { fprintf(stderr,"[TRIP] OOM\n"); exit(1); }
    ns->step_id  = step_id;
    ns->distance = dist;
    ns->left = ns->right = NULL;
    ns->height = 1;
    strncpy(ns->direction, dir, DIR_LEN - 1);
    return ns;
}

void navstep_free(NavStep *ns) { free(ns); }

/* B.1 — Clear / create new route */
void nav_clear(TripActivity *act) {
    nav_avl_free_all(act->nav_root);
    act->nav_root = NULL;
    printf("  [OK] Route cleared for activity #%d.\n", act->act_id);
}

/* B.2 — Add direction (append at end) */
int nav_add(TripActivity *act, const char *dir, float dist) {
    int new_id = nav_avl_max_id(act->nav_root) + 1;
    NavStep *ns = navstep_make(new_id, dir, dist);
    act->nav_root = nav_avl_insert(act->nav_root, ns);
    printf("  [OK] Step %d added: \"%s\" (%.2f km)\n", new_id, dir, dist);
    return 1;
}

/* B.3 — Insert direction after a given step (detour)
 * Strategy: collect all steps, rebuild the tree with new step spliced in */
int nav_insert_after(TripActivity *act, int after_id,
                     const char *dir, float dist) {
    NavStep *arr[MAX_NODES];
    int cnt = nav_avl_collect(act->nav_root, arr, MAX_NODES);

    /* validate after_id (0 = prepend) */
    if (after_id != 0) {
        int found = 0;
        for (int i = 0; i < cnt; i++)
            if (arr[i]->step_id == after_id) { found = 1; break; }
        if (!found) { printf("  [!] Step %d not found.\n", after_id); return 0; }
    }

    /* free tree structure but keep the NavStep nodes themselves */
    act->nav_root = NULL;

    /* new step node */
    NavStep *nw = navstep_make(0, dir, dist); /* id assigned below */

    int new_id = 1;
    if (after_id == 0) {
        /* prepend */
        nw->step_id = new_id++;
        nw->left = nw->right = NULL; nw->height = 1;
        act->nav_root = nav_avl_insert(act->nav_root, nw);
        for (int i = 0; i < cnt; i++) {
            arr[i]->step_id = new_id++;
            arr[i]->left = arr[i]->right = NULL; arr[i]->height = 1;
            act->nav_root = nav_avl_insert(act->nav_root, arr[i]);
        }
    } else {
        for (int i = 0; i < cnt; i++) {
            arr[i]->step_id = new_id++;
            arr[i]->left = arr[i]->right = NULL; arr[i]->height = 1;
            act->nav_root = nav_avl_insert(act->nav_root, arr[i]);
            if (arr[i]->step_id == after_id) {
                /* after_id shifted too — insert new step after current */
                nw->step_id = new_id++;
                nw->left = nw->right = NULL; nw->height = 1;
                act->nav_root = nav_avl_insert(act->nav_root, nw);
            }
        }
    }
    printf("  [OK] Step inserted after step %d. Steps renumbered.\n", after_id);
    return 1;
}

/* B.4 — Delete direction */
int nav_delete(TripActivity *act, int step_id) {
    NavStep *removed = NULL;
    act->nav_root = nav_avl_delete(act->nav_root, step_id, &removed);
    if (!removed) { printf("  [!] Step %d not found.\n", step_id); return 0; }
    free(removed);

    /* renumber remaining steps 1,2,3... */
    NavStep *arr[MAX_NODES];
    int cnt = nav_avl_collect(act->nav_root, arr, MAX_NODES);
    act->nav_root = NULL;
    for (int i = 0; i < cnt; i++) {
        arr[i]->step_id = i + 1;
        arr[i]->left = arr[i]->right = NULL; arr[i]->height = 1;
        act->nav_root = nav_avl_insert(act->nav_root, arr[i]);
    }
    printf("  [OK] Step %d deleted. Steps renumbered.\n", step_id);
    return 1;
}

/* B.5 — Search direction (text search) */
int nav_search(TripActivity *act, const char *text) {
    NavStep *found = nav_avl_search_text(act->nav_root, text);
    if (found) {
        printf("  [FOUND] Step %d: \"%s\"  (%.2f km)\n",
               found->step_id, found->direction, found->distance);
        return found->step_id;
    }
    printf("  [NOT FOUND] \"%s\" not found in navigation steps.\n", text);
    return 0;
}

/* B.6 — Update direction */
int nav_update(TripActivity *act, int step_id,
               const char *new_dir, float new_dist) {
    NavStep *ns = nav_avl_search(act->nav_root, step_id);
    if (!ns) { printf("  [!] Step %d not found.\n", step_id); return 0; }
    strncpy(ns->direction, new_dir, DIR_LEN - 1);
    ns->distance = new_dist;
    printf("  [OK] Step %d updated.\n", step_id);
    return 1;
}

/* B.7 — Display navigation route */
void nav_display(TripActivity *act) {
    printf("    Navigation to %s:\n", act->location);
    if (!act->nav_root) { printf("      (no steps)\n"); return; }
    NavStep *arr[MAX_NODES];
    int cnt = nav_avl_collect(act->nav_root, arr, MAX_NODES);
    for (int i = 0; i < cnt; i++) {
        printf("      [Step %2d]  %-55s  %.2f km\n",
               arr[i]->step_id, arr[i]->direction, arr[i]->distance);
        if (i < cnt-1) printf("               |\n");
    }
}

void nav_display_summary(TripActivity *act) {
    printf("\n  Steps for Activity #%d (%s - %s):\n",
           act->act_id, act_type_str(act->type), act->location);
    printf("  %-8s  %-55s  %s\n","StepID","Direction","Dist(km)");
    print_sep('-', 72);
    if (!act->nav_root) { printf("  (no steps)\n\n"); return; }
    NavStep *arr[MAX_NODES];
    int cnt = nav_avl_collect(act->nav_root, arr, MAX_NODES);
    for (int i = 0; i < cnt; i++)
        printf("  %-8d  %-55s  %.2f\n",
               arr[i]->step_id, arr[i]->direction, arr[i]->distance);
    printf("\n");
}
