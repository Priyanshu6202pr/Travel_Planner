#ifndef MENU_H
#define MENU_H

#include "trip.h"
#include "persistence.h"

extern TripDB     *g_db;
extern const char *g_csv;

void main_loop(void);
void menu_trip_mode(TripRecord *trip);
void menu_nav_mode(TripActivity *act);
void menu_advanced_ops(void);

#endif // MENU_H
