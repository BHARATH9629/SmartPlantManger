#ifndef DB_H
#define DB_H

/* ============================================================
   db.h — Database Layer Function Declarations
   All MySQL operations for the Smart Plant Manager
   ============================================================ */

#include "config.h"

/* Connection management */
int  db_connect(MYSQL **db);
void db_close(MYSQL *db);

/* Auto-expiry: mark plants DEAD if not watered in 18 hours */
void auto_kill(MYSQL *db);

/* API handlers — each reads request, queries DB, sends JSON response */
void api_plants(SOCKET fd, const char *body, MYSQL *db);
void api_garden(SOCKET fd, const char *body, MYSQL *db);
void api_add(SOCKET fd, const char *body, MYSQL *db);
void api_water(SOCKET fd, const char *body, MYSQL *db);
void api_remove(SOCKET fd, const char *body, MYSQL *db);
void api_bulk_water(SOCKET fd, const char *body, MYSQL *db);
void api_toggle_fav(SOCKET fd, const char *body, MYSQL *db);

/* Authentication */
void api_login(SOCKET fd, const char *body, MYSQL *db);
void api_register(SOCKET fd, const char *body, MYSQL *db);

/* Water history */
void api_water_history(SOCKET fd, const char *body, MYSQL *db);

/* Statistics */
void api_stats(SOCKET fd, const char *body, MYSQL *db);

/* Notes */
void api_add_note(SOCKET fd, const char *body, MYSQL *db);
void api_get_notes(SOCKET fd, const char *body, MYSQL *db);

/* Activity log */
void api_log_activity(MYSQL *db, int user_id, const char *action, const char *details);
void api_get_activity(SOCKET fd, const char *body, MYSQL *db);

/* Server info */
void api_server_info(SOCKET fd, const char *body, MYSQL *db);

#endif /* DB_H */
