#ifndef PLANT_H
#define PLANT_H

/* ============================================================
   plant.h  –  Data structures for Smart Plant Manager (C)
   Replaces Java classes: Plant, GardenPlant
   ============================================================ */

#define MAX_NAME   100
#define MAX_TYPE    60
#define MAX_PLANTS 200
#define MAX_GARDEN 200

/* Equivalent to Java class Plant */
typedef struct {
    int  id;
    char name[MAX_NAME];
    char plantType[MAX_TYPE];
    char soilType[MAX_TYPE];
    char scientificName[MAX_NAME];
    int  waterMl;
    int  waterIntervalHours;
    int  lifespanHours;
} Plant;

/* Equivalent to Java class GardenPlant */
typedef struct {
    int  gardenId;
    char name[MAX_NAME];
    char lastWatered[20];   /* stored as "YYYY-MM-DD HH:MM:SS" */
    int  dead;              /* 0 = alive, 1 = dead              */
    int  favorite;          /* 0 = no,    1 = yes               */
    long minutesLeft;       /* computed after fetch             */
} GardenPlant;

/* Simple dynamic-array replacements for Java ArrayList */
typedef struct {
    Plant items[MAX_PLANTS];
    int   count;
} PlantList;

typedef struct {
    GardenPlant items[MAX_GARDEN];
    int         count;
} GardenList;

/* Favourite set – replaces Java HashSet<Integer> */
typedef struct {
    int ids[MAX_GARDEN];
    int count;
} FavSet;

#endif /* PLANT_H */