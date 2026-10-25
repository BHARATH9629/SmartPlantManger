/* ============================================================
   dao.c  –  MySQL implementation of PlantDAO
   Uses:  mysql/mysql.h  (libmysql / mysqlclient)
   ============================================================ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <mysql.h>

#include "dao.h"
#include "plant.h"

/* ---- single shared connection (replaces Java DriverManager) ---- */
static MYSQL *conn = NULL;

/* -----------------------------------------------------------
   dao_connect()
   Equivalent to: DriverManager.getConnection(URL, USER, PASS)
   ----------------------------------------------------------- */
int dao_connect(const char *host, const char *user,
                const char *pass, const char *db)
{
    conn = mysql_init(NULL);
    if (!conn) {
        fprintf(stderr, "[DB] mysql_init() failed\n");
        return 0;
    }

    if (!mysql_real_connect(conn, host, user, pass, db, 0, NULL, 0)) {
        fprintf(stderr, "[DB] Connection failed: %s\n", mysql_error(conn));
        mysql_close(conn);
        conn = NULL;
        return 0;
    }

    printf("[DB] Connected to MySQL database '%s'.\n\n", db);
    return 1;
}

/* -----------------------------------------------------------
   dao_close()
   ----------------------------------------------------------- */
void dao_close(void)
{
    if (conn) {
        mysql_close(conn);
        conn = NULL;
    }
}

/* -----------------------------------------------------------
   dao_get_all_plants()
   Replaces: List<Plant> getAllPlants()
   ----------------------------------------------------------- */
PlantList dao_get_all_plants(void)
{
    PlantList list;
    list.count = 0;

    if (!conn) return list;

    const char *sql = "SELECT plant_id, plant_name, plant_type, soil_type, "
                      "scientific_name, water_amount_ml, "
                      "water_interval_hours, lifespan_hours "
                      "FROM plants ORDER BY plant_name";

    if (mysql_query(conn, sql)) {
        fprintf(stderr, "[DB] getAllPlants error: %s\n", mysql_error(conn));
        return list;
    }

    MYSQL_RES *res = mysql_store_result(conn);
    if (!res) return list;

    MYSQL_ROW row;
    while ((row = mysql_fetch_row(res)) && list.count < MAX_PLANTS) {
        Plant *p = &list.items[list.count];
        p->id                = atoi(row[0] ? row[0] : "0");
        strncpy(p->name,           row[1] ? row[1] : "", MAX_NAME-1);
        strncpy(p->plantType,      row[2] ? row[2] : "", MAX_TYPE-1);
        strncpy(p->soilType,       row[3] ? row[3] : "", MAX_TYPE-1);
        strncpy(p->scientificName, row[4] ? row[4] : "", MAX_NAME-1);
        p->waterMl            = atoi(row[5] ? row[5] : "0");
        p->waterIntervalHours = atoi(row[6] ? row[6] : "12");
        p->lifespanHours      = atoi(row[7] ? row[7] : "18");
        list.count++;
    }

    mysql_free_result(res);
    return list;
}

/* -----------------------------------------------------------
   compute_minutes_left()
   Replaces Java: Duration.between(now, lastWatered + interval)
   ----------------------------------------------------------- */
long compute_minutes_left(const char *ts, int interval_hours)
{
    if (!ts || strlen(ts) < 19) return 0;

    struct tm tm_lw = {0};
    /* Parse "YYYY-MM-DD HH:MM:SS" */
    sscanf(ts, "%4d-%2d-%2d %2d:%2d:%2d",
           &tm_lw.tm_year, &tm_lw.tm_mon, &tm_lw.tm_mday,
           &tm_lw.tm_hour, &tm_lw.tm_min, &tm_lw.tm_sec);
    tm_lw.tm_year -= 1900;
    tm_lw.tm_mon  -= 1;
    tm_lw.tm_isdst = -1;

    time_t t_lw   = mktime(&tm_lw);
    time_t t_dead = t_lw + (time_t)interval_hours * 3600;
    time_t t_now  = time(NULL);

    long diff = (long)difftime(t_dead, t_now) / 60;
    return diff < 0 ? 0 : diff;
}

/* -----------------------------------------------------------
   dao_get_garden_plants()
   Replaces: List<GardenPlant> getGardenPlants()
   ----------------------------------------------------------- */
GardenList dao_get_garden_plants(void)
{
    GardenList list;
    list.count = 0;

    if (!conn) return list;

    /* auto-kill first, just like the Java timer does */
    dao_auto_kill_expired();

    const char *sql = "SELECT garden_id, plant_name, "
                      "DATE_FORMAT(last_watered,'%Y-%m-%d %H:%i:%s'), "
                      "status "
                      "FROM my_garden ORDER BY garden_id";

    if (mysql_query(conn, sql)) {
        fprintf(stderr, "[DB] getGardenPlants error: %s\n", mysql_error(conn));
        return list;
    }

    MYSQL_RES *res = mysql_store_result(conn);
    if (!res) return list;

    MYSQL_ROW row;
    while ((row = mysql_fetch_row(res)) && list.count < MAX_GARDEN) {
        GardenPlant *gp = &list.items[list.count];
        gp->gardenId = atoi(row[0] ? row[0] : "0");
        strncpy(gp->name,        row[1] ? row[1] : "", MAX_NAME-1);
        strncpy(gp->lastWatered, row[2] ? row[2] : "", 19);
        gp->dead     = (row[3] && strcmp(row[3], "DEAD") == 0) ? 1 : 0;
        gp->favorite = 0; /* favorites are tracked in-memory (see main.c) */
        /* 15-hour window matches the Java minutesLeft() logic */
        gp->minutesLeft = compute_minutes_left(gp->lastWatered, 15);
        list.count++;
    }

    mysql_free_result(res);
    return list;
}

/* -----------------------------------------------------------
   dao_add_to_garden()
   Replaces: void addToGarden(String name)
   ----------------------------------------------------------- */
void dao_add_to_garden(const char *name)
{
    if (!conn) return;

    char sql[512];
    /* Use prepared-statement-style escaping to avoid SQL injection */
    char safe_name[MAX_NAME * 2 + 1];
    mysql_real_escape_string(conn, safe_name, name, (unsigned long)strlen(name));

    snprintf(sql, sizeof(sql),
             "INSERT INTO my_garden (plant_name) VALUES ('%s')", safe_name);

    if (mysql_query(conn, sql))
        fprintf(stderr, "[DB] addToGarden error: %s\n", mysql_error(conn));
    else
        printf("[OK] '%s' added to garden.\n", name);
}

/* -----------------------------------------------------------
   dao_water()
   Replaces: void water(int id)
   ----------------------------------------------------------- */
void dao_water(int id)
{
    if (!conn) return;

    char sql[256];
    snprintf(sql, sizeof(sql),
             "UPDATE my_garden "
             "SET last_watered=NOW(), status='ALIVE' "
             "WHERE garden_id=%d", id);

    if (mysql_query(conn, sql))
        fprintf(stderr, "[DB] water error: %s\n", mysql_error(conn));
    else
        printf("[OK] Plant #%d watered.\n", id);
}

/* -----------------------------------------------------------
   dao_remove()
   Replaces: void remove(int id)
   ----------------------------------------------------------- */
void dao_remove(int id)
{
    if (!conn) return;

    char sql[128];
    snprintf(sql, sizeof(sql),
             "DELETE FROM my_garden WHERE garden_id=%d", id);

    if (mysql_query(conn, sql))
        fprintf(stderr, "[DB] remove error: %s\n", mysql_error(conn));
    else
        printf("[OK] Plant #%d removed from garden.\n", id);
}

/* -----------------------------------------------------------
   dao_auto_kill_expired()
   Replaces Java TimerTask: UPDATE … WHERE last_watered < NOW()-18h
   ----------------------------------------------------------- */
void dao_auto_kill_expired(void)
{
    if (!conn) return;

    const char *sql =
        "UPDATE my_garden "
        "SET status='DEAD', died_on=NOW() "
        "WHERE status='ALIVE' "
        "AND last_watered < (NOW() - INTERVAL 18 HOUR)";

    if (mysql_query(conn, sql))
        fprintf(stderr, "[DB] autoKill error: %s\n", mysql_error(conn));
}