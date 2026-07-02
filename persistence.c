#include "persistence.h"
#include "utils.h"
#include <ctype.h>

static void strip(char *s) { str_strip(s); }

static char *read_file(const char *path, long *out) {
    FILE *f = fopen(path,"r"); if (!f) { *out=0; return NULL; }
    fseek(f,0,SEEK_END); long len=ftell(f); rewind(f);
    char *buf=(char*)malloc(len+2);
    size_t got=fread(buf,1,len,f); buf[got]='\0'; *out=(long)got;
    fclose(f); return buf;
}

/* ── Repair stale counter rows ──────────────────────────────────────── */
void persistence_repair(const char *path) {
    FILE *f = fopen(path,"r");
    if (!f) return;
    int has_tc=0, tc_val=1, max_tid=0;
    int max_aid[256];
    memset(max_aid,0,sizeof(max_aid));
    char line[1024];
    while (fgets(line,sizeof(line),f)) {
        strip(line);
        if (!line[0]) continue;
        if (!strncmp(line,"TRIP_COUNTER,",13)) {
            has_tc=1; tc_val=atoi(line+13);
        } else if (!strncmp(line,"TRIP,",5)) {
            char t[1024]; strncpy(t,line,1023);
            strtok(t,","); char *s=strtok(NULL,",");
            if(s){ int id=atoi(s); if(id>max_tid) max_tid=id; }
        } else if (!strncmp(line,"ACTIVITY,",9)) {
            char t[1024]; strncpy(t,line,1023);
            strtok(t,",");
            char *ts=strtok(NULL,","), *as=strtok(NULL,",");
            if(ts&&as){
                int tid=atoi(ts), aid=atoi(as);
                if(tid>=0&&tid<256&&aid>max_aid[tid]) max_aid[tid]=aid;
            }
        }
    }
    fclose(f);
    if (!(!has_tc||(tc_val<=max_tid))) return;

    long fl;
    char *content=read_file(path,&fl);
    f=fopen(path,"w");
    if(!f){ free(content); return; }
    int correct=(tc_val>max_tid+1)?tc_val:max_tid+1;
    fprintf(f,"TRIP_COUNTER,%d\n",correct);
    for(int t=1;t<=max_tid;t++) fprintf(f,"ACT_COUNTER_%d,%d\n",t,max_aid[t]+1);
    if(content){
        char *pos=content;
        while(*pos){
            char *eol=pos;
            while(*eol&&*eol!='\n') eol++;
            int len=(int)(eol-pos);
            int skip=(len>=13&&!strncmp(pos,"TRIP_COUNTER,",13))||
                     (len>=12&&!strncmp(pos,"ACT_COUNTER_",12));
            if(!skip){ fwrite(pos,1,len,f); if(*eol=='\n') fputc('\n',f); }
            pos=(*eol=='\n')?eol+1:eol;
        }
        free(content);
    }
    fclose(f);
}

/* ── Load CSV → TripDB ─────────────────────────────────────────────── */
TripDB *persistence_load(const char *path) {
    TripDB *db = trip_db_create();
    FILE *f = fopen(path,"r");
    if (!f) return db;
    char line[1024];
    while (fgets(line,sizeof(line),f)) {
        strip(line);
        if (!line[0]) continue;
        char tmp[1024]; strncpy(tmp,line,1023);
        char *tok=strtok(tmp,",");
        if(!tok) continue;

        if (!strcmp(tok,"TRIP_COUNTER")) {
            char *v=strtok(NULL,",");
            if(v) db->next_trip_id=atoi(v);
            continue;
        }
        if (!strncmp(tok,"ACT_COUNTER_",12)) {
            int tid=atoi(tok+12);
            char *v=strtok(NULL,",");
            if(v){ TripRecord *r=trip_find(db,tid); if(r) r->next_act_id=atoi(v); }
            continue;
        }
        if (!strcmp(tok,"TRIP")) {
            char *sid=strtok(NULL,","), *name=strtok(NULL,","),
                 *sl=strtok(NULL,","), *el=strtok(NULL,"\n");
            if(!sid||!name) continue;
            strip(name);
            char slo[LOC_LEN]="", elo[LOC_LEN]="";
            if(sl){ strip(sl); strncpy(slo,sl,LOC_LEN-1); }
            if(el){ strip(el); strncpy(elo,el,LOC_LEN-1); }
            /* allocate TripRecord directly — it IS the AVL node */
            TripRecord *r=(TripRecord*)calloc(1,sizeof(TripRecord));
            r->trip_id=atoi(sid); r->next_act_id=1;
            r->left=r->right=NULL; r->height=1;
            r->act_root=NULL;
            strncpy(r->name,           name, NAME_LEN-1);
            strncpy(r->start_location, slo,  LOC_LEN-1);
            strncpy(r->end_location,   elo,  LOC_LEN-1);
            if(r->trip_id>=db->next_trip_id) db->next_trip_id=r->trip_id+1;
            db->trip_root = trip_avl_insert(db->trip_root, r);
            db->count++;
            continue;
        }
        if (!strcmp(tok,"ACTIVITY")) {
            char *ts=strtok(NULL,","),*as=strtok(NULL,","),
                 *ty=strtok(NULL,","),*lo=strtok(NULL,","),
                 *dt=strtok(NULL,","),*e1=strtok(NULL,","),
                 *e2=strtok(NULL,",");
            if(!ts||!as||!ty||!lo||!dt) continue;
            strip(lo); strip(dt);
            if(e1)strip(e1); if(e2)strip(e2);
            int trip_id=atoi(ts), act_id=atoi(as);
            ActivityType atype=str_to_act_type(ty);
            ActivityDetails det; memset(&det,0,sizeof(det));
            switch(atype){
                case ACT_FLIGHT:
                    if(e1)strncpy(det.flight.flight_no,e1,29);
                    if(e2)strncpy(det.flight.airline,  e2,59); break;
                case ACT_HOTEL:
                    if(e1)strncpy(det.hotel.hotel_name,e1,59);
                    if(e2)det.hotel.charges=(float)atof(e2); break;
                case ACT_TOURIST:
                    if(e1)strncpy(det.tourist.place_name,e1,59); break;
                case ACT_TRANSPORT:
                    if(e1)strncpy(det.transport.mode,e1,39); break;
            }
            TripRecord *r=trip_find(db,trip_id);
            if(!r) continue;
            TripActivity *act=activity_make(act_id,trip_id,atype,lo,dt,det);
            r->act_root = act_avl_insert(r->act_root, act);
            if(act_id>=r->next_act_id) r->next_act_id=act_id+1;
            continue;
        }
        if (!strcmp(tok,"NAV")) {
            char *ts=strtok(NULL,","),*as=strtok(NULL,","),
                 *ss=strtok(NULL,","),*dir=strtok(NULL,","),
                 *ds=strtok(NULL,",");
            if(!ts||!as||!ss||!dir||!ds) continue;
            strip(dir);
            int tid=atoi(ts),aid=atoi(as),sid=atoi(ss);
            float dist=(float)atof(ds);
            TripRecord *r=trip_find(db,tid);
            if(!r) continue;
            TripActivity *act=trip_find_activity(r,aid);
            if(!act) continue;
            NavStep *ns=navstep_make(sid,dir,dist);
            act->nav_root = nav_avl_insert(act->nav_root, ns);
            continue;
        }
    }
    fclose(f);
    /* renumber act_ids after load */
    TripRecord *trips[MAX_NODES];
    int tcnt = trip_avl_collect(db->trip_root, trips, MAX_NODES);
    for (int i = 0; i < tcnt; i++) trip_renumber(trips[i]);
    return db;
}

/* ── Save TripDB → CSV ──────────────────────────────────────────────── */
void persistence_save(const char *path, TripDB *db) {
    FILE *f=fopen(path,"w");
    if(!f){ fprintf(stderr,"[CSV] cannot write %s\n",path); return; }
    fprintf(f,"TRIP_COUNTER,%d\n",db->next_trip_id);

    TripRecord *trips[MAX_NODES];
    int tcnt = trip_avl_collect(db->trip_root, trips, MAX_NODES);

    for(int i=0;i<tcnt;i++)
        fprintf(f,"ACT_COUNTER_%d,%d\n",trips[i]->trip_id,trips[i]->next_act_id);

    for(int i=0;i<tcnt;i++){
        TripRecord *r=trips[i];
        fprintf(f,"TRIP,%d,%s,%s,%s\n",
                r->trip_id,r->name,r->start_location,r->end_location);
        TripActivity *arr[MAX_NODES];
        int cnt=trip_collect(r,arr,MAX_NODES);
        for(int j=0;j<cnt;j++){
            TripActivity *act=arr[j];
            char e1[80]="-",e2[80]="-";
            switch(act->type){
                case ACT_FLIGHT:
                    strncpy(e1,act->details.flight.flight_no,79);
                    strncpy(e2,act->details.flight.airline,79); break;
                case ACT_HOTEL:
                    strncpy(e1,act->details.hotel.hotel_name,79);
                    snprintf(e2,79,"%.2f",act->details.hotel.charges); break;
                case ACT_TOURIST:
                    strncpy(e1,act->details.tourist.place_name,79); break;
                case ACT_TRANSPORT:
                    strncpy(e1,act->details.transport.mode,79); break;
            }
            fprintf(f,"ACTIVITY,%d,%d,%s,%s,%s,%s,%s\n",
                    r->trip_id,act->act_id,act_type_str(act->type),
                    act->location,act->date,e1,e2);
            if(act->nav_root){
                NavStep *navs[MAX_NODES];
                int nc=nav_avl_collect(act->nav_root,navs,MAX_NODES);
                for(int k=0;k<nc;k++)
                    fprintf(f,"NAV,%d,%d,%d,%s,%.2f\n",
                            r->trip_id,act->act_id,navs[k]->step_id,
                            navs[k]->direction,navs[k]->distance);
            }
        }
    }
    fclose(f);
}
