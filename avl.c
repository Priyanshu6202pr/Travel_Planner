#include "avl.h"
#include <string.h>
#include <ctype.h>

/*
 * avl.c  —  Embedded AVL Tree Implementation
 *
 * Three independent sets of AVL functions:
 *   nav_avl_*   for NavStep      (key = step_id  int)
 *   act_avl_*   for TripActivity (key = date      string)
 *   trip_avl_*  for TripRecord   (key = trip_id   int)
 *
 * Each set has its own height/balance/rotate helpers (macros used to
 * avoid repetition).  All rotations follow the same LL/RR/LR/RL pattern.
 */

/* ═══════════════════════════════════════════════════════════════════════
 * Generic height / balance macros (work for any struct with height field)
 * ═══════════════════════════════════════════════════════════════════════ */
#define HT(n)       ((n) ? (n)->height : 0)
#define UPD_HT(n)   do { int _l=HT((n)->left),_r=HT((n)->right); \
                         (n)->height = 1+(_l>_r?_l:_r); } while(0)
#define BF(n)       (HT((n)->left) - HT((n)->right))

/* ─────────────────────────────────────────────────────────────────────
 * Case-insensitive substring check  (used by nav text search)
 * ───────────────────────────────────────────────────────────────────── */
static int contains_ci(const char *hay, const char *needle) {
    char h[512], n[256];
    strncpy(h, hay,    sizeof(h)-1); h[sizeof(h)-1]='\0';
    strncpy(n, needle, sizeof(n)-1); n[sizeof(n)-1]='\0';
    for (char *p=h; *p; p++) *p=(char)tolower((unsigned char)*p);
    for (char *p=n; *p; p++) *p=(char)tolower((unsigned char)*p);
    return strstr(h,n) != NULL;
}

/* ═══════════════════════════════════════════════════════════════════════
 *  NAV AVL  —  NavStep tree, key = step_id (int)
 * ═══════════════════════════════════════════════════════════════════════ */

static NavStep *nav_rot_right(NavStep *y) {
    NavStep *x=y->left, *T2=x->right;
    x->right=y; y->left=T2;
    UPD_HT(y); UPD_HT(x); return x;
}
static NavStep *nav_rot_left(NavStep *x) {
    NavStep *y=x->right, *T2=y->left;
    y->left=x; x->right=T2;
    UPD_HT(x); UPD_HT(y); return y;
}
static NavStep *nav_balance(NavStep *n) {
    UPD_HT(n);
    int bf=BF(n);
    if (bf> 1 && BF(n->left) >=0) return nav_rot_right(n);        /* LL */
    if (bf> 1 && BF(n->left) < 0) { n->left=nav_rot_left(n->left);  return nav_rot_right(n); } /* LR */
    if (bf<-1 && BF(n->right)<=0) return nav_rot_left(n);         /* RR */
    if (bf<-1 && BF(n->right)> 0) { n->right=nav_rot_right(n->right); return nav_rot_left(n); } /* RL */
    return n;
}

NavStep *nav_avl_insert(NavStep *root, NavStep *nd) {
    if (!root) { nd->left=nd->right=NULL; nd->height=1; return nd; }
    if (nd->step_id < root->step_id)
        root->left  = nav_avl_insert(root->left,  nd);
    else if (nd->step_id > root->step_id)
        root->right = nav_avl_insert(root->right, nd);
    else
        return root;   /* duplicate — ignore */
    return nav_balance(root);
}

static NavStep *nav_min(NavStep *n) { while(n->left) n=n->left; return n; }

NavStep *nav_avl_delete(NavStep *root, int sid, NavStep **out) {
    if (!root) return NULL;
    if (sid < root->step_id)
        root->left  = nav_avl_delete(root->left,  sid, out);
    else if (sid > root->step_id)
        root->right = nav_avl_delete(root->right, sid, out);
    else {
        *out = root;
        if (!root->left || !root->right) {
            NavStep *tmp = root->left ? root->left : root->right;
            root->left = root->right = NULL;
            return tmp;
        }
        /* two children: copy successor data in, delete successor node */
        NavStep *succ = nav_min(root->right);
        /* swap content (not pointer) so root stays in place */
        int      sid_bak  = root->step_id;
        char     dir_bak[DIR_LEN]; memcpy(dir_bak, root->direction, DIR_LEN);
        float    dis_bak  = root->distance;
        root->step_id  = succ->step_id;
        memcpy(root->direction, succ->direction, DIR_LEN);
        root->distance = succ->distance;
        succ->step_id  = sid_bak;
        memcpy(succ->direction, dir_bak, DIR_LEN);
        succ->distance = dis_bak;
        /* now *out really points at the node with the original data */
        *out = succ;
        root->right = nav_avl_delete(root->right, succ->step_id, out);
        /* re-point out to the node we actually want removed */
        /* succ is now the one we detached — it has old step_id */
    }
    return nav_balance(root);
}

NavStep *nav_avl_search(NavStep *root, int sid) {
    if (!root) return NULL;
    if (sid < root->step_id) return nav_avl_search(root->left,  sid);
    if (sid > root->step_id) return nav_avl_search(root->right, sid);
    return root;
}

NavStep *nav_avl_search_text(NavStep *root, const char *needle) {
    if (!root) return NULL;
    NavStep *f = nav_avl_search_text(root->left, needle);
    if (f) return f;
    if (contains_ci(root->direction, needle)) return root;
    return nav_avl_search_text(root->right, needle);
}

int nav_avl_collect(NavStep *root, NavStep **arr, int max) {
    if (!root) return 0;
    int cnt = nav_avl_collect(root->left, arr, max);
    if (cnt < max) arr[cnt++] = root;
    cnt += nav_avl_collect(root->right, arr+cnt, max-cnt);
    return cnt;
}

int nav_avl_max_id(NavStep *root) {
    if (!root) return 0;
    int r = nav_avl_max_id(root->right);
    return r > root->step_id ? r : root->step_id;
}

void nav_avl_free_all(NavStep *root) {
    if (!root) return;
    nav_avl_free_all(root->left);
    nav_avl_free_all(root->right);
    free(root);
}

/* ═══════════════════════════════════════════════════════════════════════
 *  ACT AVL  —  TripActivity tree, key = date (string)
 * ═══════════════════════════════════════════════════════════════════════ */

static TripActivity *act_rot_right(TripActivity *y) {
    TripActivity *x=y->left, *T2=x->right;
    x->right=y; y->left=T2;
    UPD_HT(y); UPD_HT(x); return x;
}
static TripActivity *act_rot_left(TripActivity *x) {
    TripActivity *y=x->right, *T2=y->left;
    y->left=x; x->right=T2;
    UPD_HT(x); UPD_HT(y); return y;
}
static TripActivity *act_balance(TripActivity *n) {
    UPD_HT(n);
    int bf=BF(n);
    if (bf> 1 && BF(n->left) >=0) return act_rot_right(n);
    if (bf> 1 && BF(n->left) < 0) { n->left=act_rot_left(n->left);  return act_rot_right(n); }
    if (bf<-1 && BF(n->right)<=0) return act_rot_left(n);
    if (bf<-1 && BF(n->right)> 0) { n->right=act_rot_right(n->right); return act_rot_left(n); }
    return n;
}

TripActivity *act_avl_insert(TripActivity *root, TripActivity *nd) {
    if (!root) { nd->left=nd->right=NULL; nd->height=1; return nd; }
    int c = strcmp(nd->date, root->date);
    if      (c < 0) root->left  = act_avl_insert(root->left,  nd);
    else if (c > 0) root->right = act_avl_insert(root->right, nd);
    else            return root;  /* duplicate date — ignore */
    return act_balance(root);
}

static TripActivity *act_min(TripActivity *n) { while(n->left) n=n->left; return n; }

TripActivity *act_avl_delete(TripActivity *root, const char *date,
                              TripActivity **out) {
    if (!root) return NULL;
    int c = strcmp(date, root->date);
    if      (c < 0) root->left  = act_avl_delete(root->left,  date, out);
    else if (c > 0) root->right = act_avl_delete(root->right, date, out);
    else {
        *out = root;
        if (!root->left || !root->right) {
            TripActivity *tmp = root->left ? root->left : root->right;
            root->left = root->right = NULL;
            return tmp;
        }
        TripActivity *succ = act_min(root->right);
        /* swap content fields only (keep tree links intact) */
        int  aid=root->act_id; root->act_id=succ->act_id; succ->act_id=aid;
        int  tid=root->trip_id; root->trip_id=succ->trip_id; succ->trip_id=tid;
        ActivityType ty=root->type; root->type=succ->type; succ->type=ty;
        char loc[LOC_LEN]; memcpy(loc,root->location,LOC_LEN);
        memcpy(root->location,succ->location,LOC_LEN);
        memcpy(succ->location,loc,LOC_LEN);
        char dt[DATE_LEN]; memcpy(dt,root->date,DATE_LEN);
        memcpy(root->date,succ->date,DATE_LEN);
        memcpy(succ->date,dt,DATE_LEN);
        ActivityDetails det=root->details; root->details=succ->details; succ->details=det;
        NavStep *nr=root->nav_root; root->nav_root=succ->nav_root; succ->nav_root=nr;
        *out = succ;
        root->right = act_avl_delete(root->right, succ->date, out);
    }
    return act_balance(root);
}

TripActivity *act_avl_search(TripActivity *root, const char *date) {
    if (!root) return NULL;
    int c = strcmp(date, root->date);
    if (c < 0) return act_avl_search(root->left,  date);
    if (c > 0) return act_avl_search(root->right, date);
    return root;
}

int act_avl_collect(TripActivity *root, TripActivity **arr, int max) {
    if (!root) return 0;
    int cnt = act_avl_collect(root->left, arr, max);
    if (cnt < max) arr[cnt++] = root;
    cnt += act_avl_collect(root->right, arr+cnt, max-cnt);
    return cnt;
}

int act_avl_range(TripActivity *root, const char *d1, const char *d2,
                  TripActivity **arr, int max) {
    if (!root) return 0;
    int cnt = 0;
    int c1 = strcmp(root->date, d1);
    int c2 = strcmp(root->date, d2);
    if (c1 > 0) cnt += act_avl_range(root->left, d1, d2, arr, max);
    if (c1 >= 0 && c2 <= 0 && cnt < max) arr[cnt++] = root;
    if (c2 < 0)  cnt += act_avl_range(root->right, d1, d2, arr+cnt, max-cnt);
    return cnt;
}

int act_avl_size(TripActivity *root) {
    if (!root) return 0;
    return 1 + act_avl_size(root->left) + act_avl_size(root->right);
}

void act_avl_free_all(TripActivity *root) {
    if (!root) return;
    act_avl_free_all(root->left);
    act_avl_free_all(root->right);
    nav_avl_free_all(root->nav_root);
    free(root);
}

/* ═══════════════════════════════════════════════════════════════════════
 *  TRIP AVL  —  TripRecord tree, key = trip_id (int)
 * ═══════════════════════════════════════════════════════════════════════ */

static TripRecord *trip_rot_right(TripRecord *y) {
    TripRecord *x=y->left, *T2=x->right;
    x->right=y; y->left=T2;
    UPD_HT(y); UPD_HT(x); return x;
}
static TripRecord *trip_rot_left(TripRecord *x) {
    TripRecord *y=x->right, *T2=y->left;
    y->left=x; x->right=T2;
    UPD_HT(x); UPD_HT(y); return y;
}
static TripRecord *trip_balance(TripRecord *n) {
    UPD_HT(n);
    int bf=BF(n);
    if (bf> 1 && BF(n->left) >=0) return trip_rot_right(n);
    if (bf> 1 && BF(n->left) < 0) { n->left=trip_rot_left(n->left);  return trip_rot_right(n); }
    if (bf<-1 && BF(n->right)<=0) return trip_rot_left(n);
    if (bf<-1 && BF(n->right)> 0) { n->right=trip_rot_right(n->right); return trip_rot_left(n); }
    return n;
}

TripRecord *trip_avl_insert(TripRecord *root, TripRecord *nd) {
    if (!root) { nd->left=nd->right=NULL; nd->height=1; return nd; }
    if (nd->trip_id < root->trip_id)
        root->left  = trip_avl_insert(root->left,  nd);
    else if (nd->trip_id > root->trip_id)
        root->right = trip_avl_insert(root->right, nd);
    else
        return root;  /* duplicate */
    return trip_balance(root);
}

static TripRecord *trip_min(TripRecord *n) { while(n->left) n=n->left; return n; }

TripRecord *trip_avl_delete(TripRecord *root, int tid, TripRecord **out) {
    if (!root) return NULL;
    if (tid < root->trip_id)
        root->left  = trip_avl_delete(root->left,  tid, out);
    else if (tid > root->trip_id)
        root->right = trip_avl_delete(root->right, tid, out);
    else {
        *out = root;
        if (!root->left || !root->right) {
            TripRecord *tmp = root->left ? root->left : root->right;
            root->left = root->right = NULL;
            return tmp;
        }
        TripRecord *succ = trip_min(root->right);
        /* swap content */
        int id=root->trip_id; root->trip_id=succ->trip_id; succ->trip_id=id;
        char nm[NAME_LEN]; memcpy(nm,root->name,NAME_LEN);
        memcpy(root->name,succ->name,NAME_LEN); memcpy(succ->name,nm,NAME_LEN);
        char sl[LOC_LEN]; memcpy(sl,root->start_location,LOC_LEN);
        memcpy(root->start_location,succ->start_location,LOC_LEN);
        memcpy(succ->start_location,sl,LOC_LEN);
        char el[LOC_LEN]; memcpy(el,root->end_location,LOC_LEN);
        memcpy(root->end_location,succ->end_location,LOC_LEN);
        memcpy(succ->end_location,el,LOC_LEN);
        int na=root->next_act_id; root->next_act_id=succ->next_act_id; succ->next_act_id=na;
        TripActivity *ar=root->act_root; root->act_root=succ->act_root; succ->act_root=ar;
        *out = succ;
        root->right = trip_avl_delete(root->right, succ->trip_id, out);
    }
    return trip_balance(root);
}

TripRecord *trip_avl_search(TripRecord *root, int tid) {
    if (!root) return NULL;
    if (tid < root->trip_id) return trip_avl_search(root->left,  tid);
    if (tid > root->trip_id) return trip_avl_search(root->right, tid);
    return root;
}

int trip_avl_collect(TripRecord *root, TripRecord **arr, int max) {
    if (!root) return 0;
    int cnt = trip_avl_collect(root->left, arr, max);
    if (cnt < max) arr[cnt++] = root;
    cnt += trip_avl_collect(root->right, arr+cnt, max-cnt);
    return cnt;
}

void trip_avl_free_all(TripRecord *root) {
    if (!root) return;
    trip_avl_free_all(root->left);
    trip_avl_free_all(root->right);
    act_avl_free_all(root->act_root);
    free(root);
}
