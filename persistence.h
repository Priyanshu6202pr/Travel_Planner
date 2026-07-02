#ifndef PERSISTENCE_H
#define PERSISTENCE_H

#include "trip.h"

#define CSV_PATH "data.csv"

TripDB *persistence_load(const char *path);
void    persistence_save(const char *path, TripDB *db);
void    persistence_repair(const char *path);

#endif // PERSISTENCE_H
