#ifndef DAO_H
#define DAO_H

/* ============================================================
   dao.h  –  MySQL database access declarations
   Replaces Java class PlantDAO
   ============================================================ */

#include "plant.h"

/* Connect once; call dao_close() at exit */
int  dao_connect(const char *host, const char *user,
                 const char *pass, const char *db);
void dao_close(void);

/* Plant catalogue (READ-ONLY) */
PlantList  dao_get_all_plants(void);

/* Garden CRUD – mirrors every Java DAO method */
GardenList dao_get_garden_plants(void);
void       dao_add_to_garden(const char *plant_name);
void       dao_water(int garden_id);
void       dao_remove(int garden_id);
void       dao_auto_kill_expired(void);

/* Compute minutes left from a "YYYY-MM-DD HH:MM:SS" timestamp */
long compute_minutes_left(const char *last_watered_str,
                          int water_interval_hours);

#endif /* DAO_H */