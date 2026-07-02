#ifndef ADVANCED_H
#define ADVANCED_H

#include "trip.h"

// C.1 — Find path in one specific trip or across all trips
void adv_find_path_in_trip(TripRecord *trip, const char *src, const char *dst);
void adv_find_path_all(TripDB *db, const char *src, const char *dst);

// C.2 — Detect repeated locations in one trip or across all trips
void adv_detect_revisits_trip(TripRecord *trip);
void adv_detect_revisits_all(TripDB *db);

// C.3 — Sort hotels by charges descending across ALL trips
void adv_sort_hotels_all(TripDB *db);

#endif // ADVANCED_H
