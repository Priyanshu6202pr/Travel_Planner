#ifndef AVL_H
#define AVL_H

/*
 * avl.h  —  Embedded AVL Tree
 *
 * DESIGN IDEA (same as linked list but with 2 pointers instead of 1):
 *
 *   Linked list node had:   data fields  +  next*
 *   Our AVL node has:       data fields  +  left*  +  right*  +  height
 *
 * So  left / right / height  are embedded DIRECTLY inside the real structs:
 *
 *   NavStep      — keyed on step_id (int),  left/right/height inside it
 *   TripActivity — keyed on date (string),  left/right/height inside it
 *   TripRecord   — keyed on trip_id (int),  left/right/height inside it
 *
 * There is NO separate AVLTree / AVLNode wrapper struct at all.
 * The "tree" is just a pointer to the root node (TripRecord*, TripActivity*,
 * or NavStep*), exactly like a linked list is just a pointer to the head.
 *
 * Three families of functions (one per struct type):
 *   nav_avl_*   — NavStep tree
 *   act_avl_*   — TripActivity tree
 *   trip_avl_*  — TripRecord tree
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ── Constants ───────────────────────────────────────────────────────── */
#define DATE_LEN   20      /* "YYYY-MM-DD HH:MM\0" */
#define DIR_LEN    200     /* navigation direction  */
#define LOC_LEN    80
#define NAME_LEN   80
#define MAX_NODES  1024

/* forward declarations */
typedef struct TripRecord   TripRecord;
typedef struct TripActivity TripActivity;
typedef struct NavStep      NavStep;

/* ══════════════════════════════════════════════════════════════════════
 *  NavStep  (AVL node keyed on step_id)
 * ══════════════════════════════════════════════════════════════════════ */
struct NavStep {
    int   step_id;              /* AVL key (int)    */
    char  direction[DIR_LEN];
    float distance;
    /* --- embedded AVL fields (like "next" in a linked list) --- */
    NavStep *left;
    NavStep *right;
    int      height;
};

/* nav_avl functions */
NavStep *nav_avl_insert     (NavStep *root, NavStep *nd);
NavStep *nav_avl_delete     (NavStep *root, int step_id, NavStep **out);
NavStep *nav_avl_search     (NavStep *root, int step_id);
NavStep *nav_avl_search_text(NavStep *root, const char *needle);
int      nav_avl_collect    (NavStep *root, NavStep **arr, int max);
int      nav_avl_max_id     (NavStep *root);
void     nav_avl_free_all   (NavStep *root);

/* ══════════════════════════════════════════════════════════════════════
 *  Activity type / detail structs  (unchanged from original)
 * ══════════════════════════════════════════════════════════════════════ */
typedef enum {
    ACT_FLIGHT    = 0,
    ACT_HOTEL     = 1,
    ACT_TOURIST   = 2,
    ACT_TRANSPORT = 3
} ActivityType;

typedef struct { char flight_no[30]; char airline[60];  } FlightDetails;
typedef struct { char hotel_name[60]; float charges;    } HotelDetails;
typedef struct { char place_name[60];                   } TouristDetails;
typedef struct { char mode[40];                         } TransportDetails;

typedef union {
    FlightDetails    flight;
    HotelDetails     hotel;
    TouristDetails   tourist;
    TransportDetails transport;
} ActivityDetails;

/* ══════════════════════════════════════════════════════════════════════
 *  TripActivity  (AVL node keyed on date string)
 * ══════════════════════════════════════════════════════════════════════ */
struct TripActivity {
    int             act_id;           /* renumbered 1,2,3… */
    int             trip_id;
    ActivityType    type;
    char            location[LOC_LEN];
    char            date[DATE_LEN];   /* AVL key (string)  */
    ActivityDetails details;
    NavStep        *nav_root;         /* root of NavStep AVL tree  */
    /* --- embedded AVL fields --- */
    TripActivity   *left;
    TripActivity   *right;
    int             height;
};

/* act_avl functions */
TripActivity *act_avl_insert  (TripActivity *root, TripActivity *nd);
TripActivity *act_avl_delete  (TripActivity *root, const char *date,
                                TripActivity **out);
TripActivity *act_avl_search  (TripActivity *root, const char *date);
int           act_avl_collect (TripActivity *root, TripActivity **arr, int max);
int           act_avl_range   (TripActivity *root, const char *d1,
                                const char *d2, TripActivity **arr, int max);
int           act_avl_size    (TripActivity *root);
void          act_avl_free_all(TripActivity *root);

/* ══════════════════════════════════════════════════════════════════════
 *  TripRecord  (AVL node keyed on trip_id int)
 * ══════════════════════════════════════════════════════════════════════ */
struct TripRecord {
    int    trip_id;               /* AVL key (int)     */
    char   name[NAME_LEN];
    char   start_location[LOC_LEN];
    char   end_location[LOC_LEN];
    int    next_act_id;
    TripActivity *act_root;       /* root of TripActivity AVL tree */
    /* --- embedded AVL fields --- */
    TripRecord   *left;
    TripRecord   *right;
    int           height;
};

/* trip_avl functions */
TripRecord *trip_avl_insert  (TripRecord *root, TripRecord *nd);
TripRecord *trip_avl_delete  (TripRecord *root, int trip_id, TripRecord **out);
TripRecord *trip_avl_search  (TripRecord *root, int trip_id);
int         trip_avl_collect (TripRecord *root, TripRecord **arr, int max);
void        trip_avl_free_all(TripRecord *root);

/* ── TripDB ─────────────────────────────────────────────────────────── */
typedef struct {
    TripRecord *trip_root;   /* root of TripRecord AVL tree */
    int         count;
    int         next_trip_id;
} TripDB;

#endif /* AVL_H */
