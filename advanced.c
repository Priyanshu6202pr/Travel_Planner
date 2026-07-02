#include "advanced.h"
#include "utils.h"
#include <stdlib.h>

/* ── Helper: print activities from src to dst inside one trip ───────── */
static int find_path_in_trip(TripRecord *trip, const char *src, const char *dst) {
    TripActivity *arr[MAX_NODES];
    int cnt = trip_collect(trip, arr, MAX_NODES);
    int started = 0, finished = 0;
    for (int i = 0; i < cnt; i++) {
        TripActivity *act = arr[i];
        if (!started && str_contains_ci(act->location, src)) started = 1;
        if (!started) continue;
        printf("\n    [%d] %-10s  %-25s  (%s)\n",
               act->act_id, act_type_str(act->type), act->location, act->date);
        switch (act->type) {
            case ACT_FLIGHT:
                printf("        Flight %s via %s\n",
                       act->details.flight.flight_no, act->details.flight.airline); break;
            case ACT_HOTEL:
                printf("        Hotel: %s  (INR %.2f)\n",
                       act->details.hotel.hotel_name, act->details.hotel.charges); break;
            case ACT_TOURIST:
                printf("        Place: %s\n", act->details.tourist.place_name); break;
            case ACT_TRANSPORT:
                printf("        Mode: %s\n",  act->details.transport.mode); break;
        }
        if (act->nav_root) nav_display(act);
        if (str_contains_ci(act->location, dst)) {
            finished = 1;
            printf("\n    [OK] Destination [%s] reached.\n", dst);
            break;
        }
    }
    if (!started)  { printf("    [!] Source [%s] not found in this trip.\n\n", src);          return 0; }
    if (!finished) { printf("    [!] Destination [%s] not reachable from source.\n\n", dst);  return 0; }
    return 1;
}

/* ── C.1a ───────────────────────────────────────────────────────────── */
void adv_find_path_in_trip(TripRecord *trip, const char *src, const char *dst) {
    printf("\n  Finding path: [%s] -> [%s]  in Trip #%d (%s)\n",
           src, dst, trip->trip_id, trip->name);
    print_sep('-', 64);
    if (!find_path_in_trip(trip, src, dst))
        printf("  Path [%s] -> [%s] does not exist in this trip.\n\n", src, dst);
}

/* ── C.1b ───────────────────────────────────────────────────────────── */
void adv_find_path_all(TripDB *db, const char *src, const char *dst) {
    printf("\n  Searching path [%s] -> [%s] across all trips:\n", src, dst);
    print_sep('-', 64);
    TripRecord *trips[MAX_NODES];
    int tcnt = trip_avl_collect(db->trip_root, trips, MAX_NODES);
    int found_any = 0;
    for (int i = 0; i < tcnt; i++) {
        printf("\n  -- Trip #%d : %s --\n", trips[i]->trip_id, trips[i]->name);
        if (find_path_in_trip(trips[i], src, dst)) found_any = 1;
    }
    if (!found_any)
        printf("\n  [!] No path [%s] -> [%s] found in any trip.\n\n", src, dst);
}

/* ── C.2a ───────────────────────────────────────────────────────────── */
void adv_detect_revisits_trip(TripRecord *trip) {
    TripActivity *arr[MAX_NODES];
    int cnt = trip_collect(trip, arr, MAX_NODES);
    printf("\n  Multiple-Visit Detection — Trip #%d (%s)\n", trip->trip_id, trip->name);
    print_sep('-', 54);
    char locs[MAX_NODES][LOC_LEN]; int freq[MAX_NODES]; int total=0;
    for (int i = 0; i < cnt; i++) {
        int found=0;
        for (int j = 0; j < total; j++) {
            if (str_contains_ci(locs[j],arr[i]->location) &&
                str_contains_ci(arr[i]->location,locs[j])) { freq[j]++; found=1; break; }
        }
        if (!found && total<MAX_NODES) {
            strncpy(locs[total],arr[i]->location,LOC_LEN-1); freq[total++]=1;
        }
    }
    int any=0;
    for (int i=0;i<total;i++) if(freq[i]>1) { printf("  %-40s  visited %d time(s)\n",locs[i],freq[i]); any=1; }
    if (!any) printf("  No location visited more than once.\n");
    printf("\n");
}

/* ── C.2b ───────────────────────────────────────────────────────────── */
void adv_detect_revisits_all(TripDB *db) {
    printf("\n  Multiple-Visit Detection — All Trips\n");
    print_sep('-', 54);
    char locs[MAX_NODES][LOC_LEN]; int freq[MAX_NODES]; int total=0;
    TripRecord *trips[MAX_NODES];
    int tcnt = trip_avl_collect(db->trip_root, trips, MAX_NODES);
    for (int t = 0; t < tcnt; t++) {
        TripActivity *arr[MAX_NODES];
        int cnt = trip_collect(trips[t], arr, MAX_NODES);
        for (int i = 0; i < cnt; i++) {
            int found=0;
            for (int j=0;j<total;j++) {
                if (str_contains_ci(locs[j],arr[i]->location) &&
                    str_contains_ci(arr[i]->location,locs[j])) { freq[j]++; found=1; break; }
            }
            if (!found && total<MAX_NODES) {
                strncpy(locs[total],arr[i]->location,LOC_LEN-1); freq[total++]=1;
            }
        }
    }
    int any=0;
    for (int i=0;i<total;i++) if(freq[i]>1) { printf("  %-40s  visited %d time(s)\n",locs[i],freq[i]); any=1; }
    if (!any) printf("  No location visited more than once across all trips.\n");
    printf("\n");
}

/* ── C.3 ────────────────────────────────────────────────────────────── */
typedef struct { char hotel[60]; char loc[LOC_LEN]; char trip_name[NAME_LEN]; int trip_id; float charges; } HotelEntry;
static int cmp_hotel(const void *a, const void *b) {
    float ca=((HotelEntry*)a)->charges, cb=((HotelEntry*)b)->charges;
    return (cb>ca)-(cb<ca);
}

void adv_sort_hotels_all(TripDB *db) {
    HotelEntry hotels[MAX_NODES]; int hcnt=0;
    TripRecord *trips[MAX_NODES];
    int tcnt = trip_avl_collect(db->trip_root, trips, MAX_NODES);
    for (int t=0;t<tcnt&&hcnt<MAX_NODES;t++) {
        TripActivity *arr[MAX_NODES];
        int cnt = trip_collect(trips[t], arr, MAX_NODES);
        for (int i=0;i<cnt&&hcnt<MAX_NODES;i++) {
            if (arr[i]->type==ACT_HOTEL) {
                strncpy(hotels[hcnt].hotel,    arr[i]->details.hotel.hotel_name,59);
                strncpy(hotels[hcnt].loc,       arr[i]->location,LOC_LEN-1);
                strncpy(hotels[hcnt].trip_name, trips[t]->name,NAME_LEN-1);
                hotels[hcnt].trip_id = trips[t]->trip_id;
                hotels[hcnt].charges = arr[i]->details.hotel.charges;
                hcnt++;
            }
        }
    }
    if (!hcnt) { printf("\n  No hotel activities found in any trip.\n\n"); return; }
    qsort(hotels,hcnt,sizeof(HotelEntry),cmp_hotel);
    printf("\n  Hotels sorted by charges (descending) — All Trips:\n");
    print_sep('-',80);
    printf("  %-4s  %-25s  %-22s  %-20s  %-10s  %s\n","Rank","Hotel","Location","Trip Name","TripID","INR");
    print_sep('-',80);
    for (int i=0;i<hcnt;i++)
        printf("  #%-3d  %-25s  %-22s  %-20s  %-10d  %.2f\n",
               i+1,hotels[i].hotel,hotels[i].loc,hotels[i].trip_name,hotels[i].trip_id,hotels[i].charges);
    printf("\n");
}
