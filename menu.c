#include "menu.h"
#include "advanced.h"
#include "utils.h"
#include <ctype.h>

#define clrscr() printf("\033[2J\033[H")

TripDB     *g_db  = NULL;
const char *g_csv = CSV_PATH;

// ── Collect activity details ─────────────────────────────────────────────
static ActivityDetails collect_details(ActivityType t) {
    ActivityDetails det; memset(&det,0,sizeof(det));
    char buf[256];
    switch(t){
        case ACT_FLIGHT:
            printf("  Flight No : "); 
            read_line(buf,sizeof(buf)); 
            strncpy(det.flight.flight_no,buf,29);
            printf("  Airline   : "); 
            read_line(buf,sizeof(buf)); 
            strncpy(det.flight.airline,  buf,59);
            break;
        case ACT_HOTEL:
            printf("  Hotel name: "); 
            read_line(buf,sizeof(buf)); 
            strncpy(det.hotel.hotel_name,buf,59);
            det.hotel.charges=read_float("  Charges (INR): ");
            break;
        case ACT_TOURIST:
            printf("  Place name: "); 
            read_line(buf,sizeof(buf)); 
            strncpy(det.tourist.place_name,buf,59);
            break;
        case ACT_TRANSPORT:
            printf("  Mode (Taxi/Metro/Bus): "); 
            read_line(buf,sizeof(buf)); 
            strncpy(det.transport.mode,buf,39);
            break;
    }
    return det;
}


// static ActivityType choose_type(void) {
//     printf("  Type: 0=FLIGHT  1=HOTEL  2=TOURIST  3=TRANSPORT\n");
//     int t=read_int("  Choice: ");
//     if(t<0||t>3) t=0;
//     return (ActivityType)t;
// }

static ActivityType choose_type(void) {
    int t;
    do {
        printf("  Type: 0=FLIGHT  1=HOTEL  2=TOURIST  3=TRANSPORT\n");
        t = read_int("  Choice: ");
        if (t < 0 || t > 3)
            printf("  [!] Invalid choice. Try again.\n");
    } while (t < 0 || t > 3);

    return (ActivityType)t;
}


// ══════════════════════════════════════════════════════════════════════════
// Navigation sub-menu  (Section B — persistent loop)
// ══════════════════════════════════════════════════════════════════════════
void menu_nav_mode(TripActivity *act) {
    char buf[256];
    int run=1;
    while(run){
        clrscr();
        printf("\n"); print_sep('-',58);
        printf("  Navigation — Act #%d | %s | %s\n",
               act->act_id, act_type_str(act->type), act->location);
        print_sep('-',58);
        printf("  1. Add direction (append)\n");
        printf("  2. Insert direction (after a step)\n");
        printf("  3. Delete direction\n");
        printf("  4. Search direction\n");
        printf("  5. Update direction\n");
        printf("  6. Display navigation route\n");
        printf("  7. Clear / create new route\n");
        printf("  0. Back\n");
        print_sep('-',58);

        int ch=read_int("  Choice: ");
        switch(ch){
            case 1: {
                printf("  Direction: "); read_line(buf,sizeof(buf));
                float d=read_float("  Distance (km): ");
                nav_add(act,buf,d);
                persistence_save(g_csv,g_db); break;
            }
            case 2: {
                nav_display_summary(act);
                int after=read_int("  Insert AFTER step ID (0=prepend): ");
                printf("  Direction: "); 
                read_line(buf,sizeof(buf));
                float d=read_float("  Distance (km): ");
                nav_insert_after(act,after,buf,d);
                persistence_save(g_csv,g_db); break;
            }
            case 3: {
                nav_display_summary(act);
                int sid=read_int("  Step ID to delete: ");
                nav_delete(act,sid);
                persistence_save(g_csv,g_db); break;
            }
            case 4:
                printf("  Search text: "); read_line(buf,sizeof(buf));
                nav_search(act,buf); break;
            case 5: {
                nav_display_summary(act);
                int sid=read_int("  Step ID to update: ");
                printf("  New direction: "); read_line(buf,sizeof(buf));
                float d=read_float("  New distance (km): ");
                nav_update(act,sid,buf,d);
                persistence_save(g_csv,g_db); break;
            }
            case 6: nav_display(act); break;
            case 7: nav_clear(act); persistence_save(g_csv,g_db); break;
            case 0: run=0; break;
            default: printf("  [!] Invalid.\n");
        }
    }
}

// ══════════════════════════════════════════════════════════════════════════
// Open Trip menu  (replaces old menu_trip_mode, now option O in main)
// Options per spec:
//   1. Add activity
//   2. Insert activity (chronological by date)
//   3. Delete activity
//   4. Display full trip plan
//   5. Display summary table
//   6. Manage navigation steps
//   7. Range search (D1 → D2)
//   0. Back to main menu
// ══════════════════════════════════════════════════════════════════════════
void menu_trip_mode(TripRecord *trip) {
    char buf[256];
    int run=1;
    while(run){
        clrscr();
        printf("\n"); 
        print_sep('=',64);
        printf("  Trip #%d : %s\n", trip->trip_id, trip->name);
        printf("  From    : %s  →  To: %s\n",
               trip->start_location[0] ? trip->start_location : "(not set)",
               trip->end_location[0]   ? trip->end_location   : "(not set)");
        printf("  Activities: %d\n", act_avl_size(trip->act_root));
        print_sep('=',64);
        printf("  1. Add activity\n");
        printf("  2. Insert activity (chronological by date)\n");
        printf("  3. Delete activity\n");
        printf("  4. Display full trip plan\n");
        printf("  5. Display summary table\n");
        printf("  6. Manage navigation steps\n");
        printf("  7. Range search (D1 → D2)\n");
        printf("  0. Back to main menu\n");
        print_sep('=',64);
        int ch=read_int("  Choice: ");

        switch(ch){
            // A.1 Add  /  A.2 Insert — both go through AVL insert (auto-sorted by date)
            case 1:
            case 2: {
                ActivityType atype=choose_type();
                printf("  Location: "); 
                read_line(buf,sizeof(buf));
                char loc[LOC_LEN]; 
                strncpy(loc,buf,LOC_LEN-1);
                char date[DATE_LEN];
                do {
                    printf("  Date (YYYY-MM-DD HH:MM): "); 
                    read_line(date,DATE_LEN);
                    if(!date_valid(date)) printf("  [!] Invalid format.\n");
                } while(!date_valid(date));

                ActivityDetails det=collect_details(atype);
                TripActivity *act=activity_make(trip->next_act_id,trip->trip_id,atype,loc,date,det);
                trip_add_activity(trip,act);     // AVL insert — auto-ordered
                trip_renumber(trip);             // dynamic IDs 1,2,3...

                // Sync start/end location of trip with first/last activity
                TripActivity *all[MAX_NODES];
                int total=trip_collect(trip,all,MAX_NODES);
                if(total>0 && !trip->start_location[0])
                    strncpy(trip->start_location, all[0]->location, LOC_LEN-1);
                if(total>0)
                    strncpy(trip->end_location, all[total-1]->location, LOC_LEN-1);

                persistence_save(g_csv,g_db);
                printf("  [OK] Activity added (ActID #%d).\n",act->act_id);
                // Immediately offer nav sub-menu
                printf("\n  Add navigation steps now? (1=Yes / 0=No): ");
                if(read_int("")==1) menu_nav_mode(act);
                break;
            }
            // A.3 Delete
            case 3: {
                trip_display_summary(trip);
                int aid=read_int("  ActID to delete (0=cancel): ");
                if(!aid) break;
                if(trip_delete_activity(trip,aid)){
                    printf("  [OK] Activity #%d deleted and IDs renumbered.\n",aid);
                    persistence_save(g_csv,g_db);
                } else printf("  [!] Activity #%d not found.\n",aid);
                break;
            }
            // A.4 Display full
            case 4: trip_display(trip); break;
            // A.5 Display summary
            case 5: trip_display_summary(trip); 
            break;
            // Navigation sub-menu
            case 6: {
                trip_display_summary(trip);
                int aid=read_int("  ActID to manage nav (0=cancel): ");
                if(!aid) break;
                TripActivity *act=trip_find_activity(trip,aid);
                if(!act){printf("  [!] Not found.\n");
                    break;
                }
                menu_nav_mode(act); 
                break;
            }
            // Range search — Important New Operation (PDF)
            case 7: {
                char d1[DATE_LEN], d2[DATE_LEN];
                printf("  Start date (YYYY-MM-DD HH:MM): "); read_line(d1, DATE_LEN);
                printf("  End   date (YYYY-MM-DD HH:MM): "); read_line(d2, DATE_LEN);
                if (!date_valid(d1) || !date_valid(d2)) {
                    printf("  [!] Invalid date format. Use YYYY-MM-DD HH:MM\n"); 
                    break;
                }
                if (strcmp(d1, d2) > 0) {
                    printf("  [!] Start date must be <= End date.\n"); 
                    break;
                }
                TripActivity *res[MAX_NODES];
                int cnt = trip_range_search(trip, d1, d2, res, MAX_NODES);
                printf("\n  Range [%s] → [%s] : %d result(s)\n", d1, d2, cnt);
                print_sep('-', 75);
                if (!cnt) { printf("  No activities found in this date range.\n\n"); 
                    break; }
                printf("  %-6s  %-10s  %-25s  %-18s  %s\n",
                       "ActID","Type","Location","Date","Detail");
                print_sep('-', 75);
                for (int i = 0; i < cnt; i++) {
                    TripActivity *act = res[i];
                    char det[80] = "-";
                    switch (act->type) {
                        case ACT_FLIGHT:
                            snprintf(det,79,"%.14s / %.14s",
                                     act->details.flight.flight_no,
                                     act->details.flight.airline); break;
                        case ACT_HOTEL:
                            snprintf(det,79,"%.20s INR %.0f",
                                     act->details.hotel.hotel_name,
                                     act->details.hotel.charges); break;
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
                break;
            }
            case 0: run=0; break;
            default: printf("  [!] Invalid.\n");
        }
    }
}

// ══════════════════════════════════════════════════════════════════════════
// Helper — print all trips with IDs so user can pick one
// ══════════════════════════════════════════════════════════════════════════
static void list_trips_brief(void) {
    if (!g_db->trip_root) {
        printf("  (no trips)\n");
        return;
    }
    printf("  %-6s  %-30s  %-20s  %-20s\n","TripID","Name","From","To");
    print_sep('-', 80);
    TripRecord *arr[MAX_NODES];
    int cnt = trip_avl_collect(g_db->trip_root, arr, MAX_NODES);
    for (int i = 0; i < cnt; i++) {
        TripRecord *r = arr[i];
        printf("  %-6d  %-30s  %-20s  %-20s\n",
               r->trip_id, r->name,
               r->start_location[0] ? r->start_location : "-",
               r->end_location[0]   ? r->end_location   : "-");
    }
    printf("\n");
}

// ══════════════════════════════════════════════════════════════════════════
// Advanced operations menu  (Section A in main menu)
// Sub-options:
//   1. Find path (source → destination)
//   2. Detect repeated locations
//   3. Sort hotels by charges (descending)
//   0. Back
// ══════════════════════════════════════════════════════════════════════════
void menu_advanced_ops(void) {
    char buf1[80], buf2[80];
    int run=1;
    while(run){
        clrscr();
        print_section("Advanced Operations");
        printf("  1. Find path (source → destination)\n");
        printf("  2. Detect repeated locations\n");
        printf("  3. Sort hotels by charges (descending)\n");
        printf("  0. Back\n");
        print_sep('-',52);
        int ch=read_int("  Choice: ");

        switch(ch){
            // ── C.1 Find path ─────────────────────────────────────────────
            case 1: {
                printf("  Source     : "); read_line(buf1,sizeof(buf1));
                printf("  Destination: "); read_line(buf2,sizeof(buf2));
                if(!buf1[0]||!buf2[0]){ printf("  [!] Source/Destination cannot be empty.\n"); break; }

                printf("\n  Search in:\n");
                printf("    1. A specific trip\n");
                printf("    2. All trips\n");
                int scope=read_int("  Choice: ");

                if(scope==1){
                    list_trips_brief();
                    int tid=read_int("  Enter Trip ID (0=cancel): ");
                    if(!tid) break;
                    TripRecord *r=trip_find(g_db,tid);
                    if(!r){ printf("  [!] Trip #%d not found.\n",tid); 
                        break; 
                    }
                    adv_find_path_in_trip(r,buf1,buf2);
                } else if(scope==2){
                    adv_find_path_all(g_db,buf1,buf2);
                } else {
                    printf("  [!] Invalid choice.\n");
                }
                break;
            }
            // ── C.2 Detect repeated locations ──────────────────────────────
            case 2: {
                printf("\n  Search in:\n");
                printf("    1. A specific trip\n");
                printf("    2. All trips\n");
                int scope=read_int("  Choice: ");

                if(scope==1){
                    list_trips_brief();
                    int tid=read_int("  Enter Trip ID (0=cancel): ");
                    if(!tid) break;
                    TripRecord *r=trip_find(g_db,tid);
                    if(!r){ printf("  [!] Trip #%d not found.\n",tid); 
                        break; 
                    }
                    adv_detect_revisits_trip(r);
                } else if(scope==2){
                    adv_detect_revisits_all(g_db);
                } else {
                    printf("  [!] Invalid choice.\n");
                }
                break;
            }
            // ── C.3 Sort hotels across all trips ───────────────────────────
            case 3:
                adv_sort_hotels_all(g_db);
                break;
            case 0: run=0; break;
            default: printf("  [!] Invalid.\n");
        }
    }
}

// ══════════════════════════════════════════════════════════════════════════
// Main loop
//   L. List all trips
//   N. New trip
//   O. Open trip
//   A. Advanced operations
//   Q. Save & Quit
// ══════════════════════════════════════════════════════════════════════════
void main_loop(void) {
    char buf[128];
    int run=1;
    while(run){
        clrscr();
        print_header("TRAVEL PLANNER  [AVL Tree Edition]");
        printf("  L. List all trips\n");
        printf("  N. New trip\n");
        printf("  O. Open trip\n");
        printf("  A. Advanced operations\n");
        printf("  Q. Save & Quit\n");
        print_sep('=',62);
        printf("  Choice: "); 
        read_line(buf,sizeof(buf));
        char key=(char)toupper((unsigned char)buf[0]);

        switch(key){
            // ── L: List ───────────────────────────────────────────────────
            case 'L':
                print_section("All Trips");
                trip_list(g_db);
                break;

            // ── N: New trip ───────────────────────────────────────────────
            case 'N': {
                printf("  Trip name         : "); 
                read_line(buf,sizeof(buf));
                if(!buf[0]){ 
                    printf("  [!] Name cannot be empty.\n"); 
                    break; 
                }
                char trip_name[NAME_LEN];
                strncpy(trip_name,buf,NAME_LEN-1);

                char start_loc[LOC_LEN], end_loc[LOC_LEN];
                printf("  Starting location : "); 
                read_line(start_loc,sizeof(start_loc));
                printf("  Final destination : "); 
                read_line(end_loc,  sizeof(end_loc));

                TripRecord *r=trip_create(g_db,trip_name,start_loc,end_loc);
                persistence_save(g_csv,g_db);
                printf("  [OK] Trip #%d '%s' created.  (%s → %s)\n",
                       r->trip_id, r->name,
                       r->start_location[0] ? r->start_location : "-",
                       r->end_location[0]   ? r->end_location   : "-");
                // Drop straight into the trip menu
                menu_trip_mode(r);
                break;
            }

            // ── O: Open trip ──────────────────────────────────────────────
            case 'O': {
                print_section("All Trips");
                trip_list(g_db);
                int tid=read_int("  Trip ID to open (0=cancel): ");
                if(!tid) break;
                TripRecord *r=trip_find(g_db,tid);
                if(!r){ 
                    printf("  [!] Trip #%d not found.\n",tid); 
                    break; 
                }
                menu_trip_mode(r);
                break;
            }

            // ── A: Advanced ops ───────────────────────────────────────────
            case 'A':
                menu_advanced_ops();
                break;

            // ── Q: Quit ─────────────────────────────────────────────
            case 'Q': run=0; break;

            default: printf("  [!] Invalid option.\n");
        }
    }
}
