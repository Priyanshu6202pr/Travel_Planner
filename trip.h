#ifndef TRIP_H
#define TRIP_H

/*
 * trip.h  —  Trip-level and Navigation-level API
 *
 * All data structs (NavStep, TripActivity, TripRecord, TripDB) are defined
 * in avl.h because the AVL left/right/height fields are embedded directly
 * inside those structs.
 *
 * This file only declares the functions that operate on those structs.
 */

#include "avl.h"

/* ── TripDB lifecycle ─────────────────────────────────────────────── */
TripDB *trip_db_create  (void);
void    trip_db_destroy (TripDB *db);

/* ── TripRecord operations ────────────────────────────────────────── */
TripRecord  *trip_create  (TripDB *db, const char *name,
                            const char *start_loc, const char *end_loc);
TripRecord  *trip_find    (TripDB *db, int trip_id);
void         trip_list    (TripDB *db);

/* ── TripActivity operations ──────────────────────────────────────── */
TripActivity *activity_make  (int act_id, int trip_id, ActivityType type,
                               const char *location, const char *date,
                               ActivityDetails details);
void          activity_free  (TripActivity *act);

int  trip_add_activity    (TripRecord *trip, TripActivity *act);
int  trip_delete_activity (TripRecord *trip, int act_id);
void trip_renumber        (TripRecord *trip);
int  trip_collect         (TripRecord *trip, TripActivity **arr, int max);
TripActivity *trip_find_activity (TripRecord *trip, int act_id);
int  trip_range_search    (TripRecord *trip, const char *d1, const char *d2,
                            TripActivity **out, int max);
void trip_display         (TripRecord *trip);
void trip_display_summary (TripRecord *trip);

/* ── Utility ──────────────────────────────────────────────────────── */
const char  *act_type_str   (ActivityType t);
ActivityType str_to_act_type(const char *s);

/* ── Navigation operations ────────────────────────────────────────── */
NavStep *navstep_make   (int step_id, const char *dir, float dist);
void     navstep_free   (NavStep *ns);

int  nav_add          (TripActivity *act, const char *dir, float dist);
int  nav_insert_after (TripActivity *act, int after_id,
                        const char *dir, float dist);
int  nav_delete       (TripActivity *act, int step_id);
int  nav_update       (TripActivity *act, int step_id,
                        const char *new_dir, float new_dist);
int  nav_search       (TripActivity *act, const char *text);
void nav_display      (TripActivity *act);
void nav_display_summary(TripActivity *act);
void nav_clear        (TripActivity *act);

#endif /* TRIP_H */
