/* ============================================================
   db.c — Database Layer Implementation
   All MySQL operations for Smart Plant Manager v2
   Demonstrates: MySQL C API, dynamic memory, error handling,
                 SQL injection prevention, string building
   ============================================================ */

#include "db.h"
#include "http.h"

/* ── Extern globals from server.c ── */
extern volatile long g_total_requests;
extern time_t        g_start_time;

/* ═══════════════════════════════════
   DATABASE CONNECTION
═══════════════════════════════════ */
int db_connect(MYSQL **db) {
    *db = mysql_init(NULL);
    if (!*db) {
        fprintf(stderr, "[ERROR] mysql_init() failed\n");
        return 0;
    }
    if (!mysql_real_connect(*db, DB_HOST, DB_USER, DB_PASS, DB_NAME, 0, NULL, 0)) {
        fprintf(stderr, "[ERROR] MySQL: %s\n", mysql_error(*db));
        mysql_close(*db);
        *db = NULL;
        return 0;
    }
    /* Set UTF-8 charset */
    mysql_set_character_set(*db, "utf8mb4");
    return 1;
}

void db_close(MYSQL *db) {
    if (db) mysql_close(db);
}

/* ═══════════════════════════════════
   AUTO KILL — Mark expired plants DEAD
   Plants die if not watered within 18 hours
═══════════════════════════════════ */
void auto_kill(MYSQL *db) {
    if (!db) return;
    if (mysql_query(db,
        "UPDATE my_garden SET status='DEAD', died_on=NOW() "
        "WHERE status='ALIVE' AND last_watered < (NOW() - INTERVAL 18 HOUR)")) {
        fprintf(stderr, "[ERROR] auto_kill: %s\n", mysql_error(db));
    }
}

/* ═══════════════════════════════════
   HELPER: Generate random session ID
   Creates a 32-char hex string for session tokens
═══════════════════════════════════ */
static void generate_session_id(char *out, int len) {
    /* Seed the thread-local rand() for this worker thread */
#ifdef _WIN32
    unsigned int seed = (unsigned int)(time(NULL) ^ GetCurrentProcessId() ^ GetCurrentThreadId());
#else
    unsigned int seed = (unsigned int)(time(NULL) ^ (unsigned int)getpid() ^ (unsigned int)(uintptr_t)pthread_self());
#endif
    seed ^= (unsigned int)(uintptr_t)&out;
    srand(seed);

    static const char hex[] = "0123456789abcdef";
    for (int i = 0; i < len - 1; i++) {
        out[i] = hex[rand() % 16];
    }
    out[len - 1] = '\0';
}

static int get_user_id_from_session(MYSQL *db, const char *session_id) {
    if (!db || !session_id || session_id[0] == '\0') return -1;
    char sql[512];
    snprintf(sql, sizeof(sql),
             "SELECT user_id FROM sessions WHERE session_id = '%s' AND expires_at > NOW() LIMIT 1",
             session_id);
    if (mysql_query(db, sql)) return -1;
    MYSQL_RES *res = mysql_store_result(db);
    if (!res) return -1;
    MYSQL_ROW row = mysql_fetch_row(res);
    int uid = -1;
    if (row && row[0]) uid = atoi(row[0]);
    mysql_free_result(res);
    return uid;
}

static int validate_session(SOCKET fd, MYSQL *db, const char *req_or_body, int is_get) {
    char token[64] = {0};
    if (is_get) {
        get_query_param(req_or_body, "session", token, sizeof(token));
    } else {
        jget(req_or_body, "session", token, sizeof(token));
    }
    int uid = get_user_id_from_session(db, token);
    if (uid == -1) {
        send_resp(fd, "401 Unauthorized", "application/json", "{\"error\":\"Unauthorized\"}");
        return -1;
    }
    return uid;
}

/* ═══════════════════════════════════
   API: GET /api/plants — All plants with full info
═══════════════════════════════════ */
void api_plants(SOCKET fd, const char *body, MYSQL *db) {
    (void)body; /* unused for GET */
    if (!db) { send_resp(fd, "500 Error", "application/json", "[]"); return; }

    if (mysql_query(db,
        "SELECT plant_id, plant_name, plant_type, soil_type, scientific_name, "
        "water_amount_ml, water_interval_hours, lifespan_hours, "
        "sunlight, difficulty, best_season, benefits, description "
        "FROM plants ORDER BY plant_name")) {
        fprintf(stderr, "[ERROR] api_plants: %s\n", mysql_error(db));
        send_resp(fd, "500 Error", "application/json", "[]");
        return;
    }

    MYSQL_RES *res = mysql_store_result(db);
    if (!res) { send_resp(fd, "500 Error", "application/json", "[]"); return; }

    /* Dynamic JSON builder using malloc/realloc */
    int cap = 65536, len = 0;
    char *buf = (char *)malloc(cap);
    if (!buf) { mysql_free_result(res); send_resp(fd, "500 Error", "application/json", "[]"); return; }
    buf[len++] = '[';

    MYSQL_ROW row;
    int first = 1;
    while ((row = mysql_fetch_row(res))) {
        /* Escape fields for safe JSON */
        char esc_name[256]={0}, esc_type[128]={0}, esc_soil[128]={0}, esc_sci[320]={0};
        char esc_sun[128]={0}, esc_diff[64]={0}, esc_season[128]={0};
        char esc_benefits[1024]={0}, esc_desc[1024]={0};

        json_escape(row[1]?row[1]:"", esc_name, sizeof(esc_name));
        json_escape(row[2]?row[2]:"", esc_type, sizeof(esc_type));
        json_escape(row[3]?row[3]:"", esc_soil, sizeof(esc_soil));
        json_escape(row[4]?row[4]:"", esc_sci, sizeof(esc_sci));
        json_escape(row[8]?row[8]:"", esc_sun, sizeof(esc_sun));
        json_escape(row[9]?row[9]:"", esc_diff, sizeof(esc_diff));
        json_escape(row[10]?row[10]:"", esc_season, sizeof(esc_season));
        json_escape(row[11]?row[11]:"", esc_benefits, sizeof(esc_benefits));
        json_escape(row[12]?row[12]:"", esc_desc, sizeof(esc_desc));

        char item[4096];
        int n = snprintf(item, sizeof(item),
            "%s{\"id\":%s,\"name\":\"%s\",\"type\":\"%s\",\"soil\":\"%s\","
            "\"scientific\":\"%s\",\"water_ml\":%s,\"interval\":%s,\"lifespan\":%s,"
            "\"sunlight\":\"%s\",\"difficulty\":\"%s\",\"best_season\":\"%s\","
            "\"benefits\":\"%s\",\"description\":\"%s\"}",
            first ? "" : ",",
            row[0]?row[0]:"0", esc_name, esc_type, esc_soil, esc_sci,
            row[5]?row[5]:"0", row[6]?row[6]:"12", row[7]?row[7]:"18",
            esc_sun, esc_diff, esc_season, esc_benefits, esc_desc);
        first = 0;

        /* Grow buffer if needed — demonstrates realloc */
        while (len + n + 2 >= cap) {
            cap *= 2;
            char *tmp = (char *)realloc(buf, cap);
            if (!tmp) { free(buf); mysql_free_result(res); send_resp(fd, "500 Error", "application/json", "[]"); return; }
            buf = tmp;
        }
        memcpy(buf + len, item, n);
        len += n;
    }
    mysql_free_result(res);

    buf[len++] = ']';
    buf[len] = '\0';
    send_resp(fd, "200 OK", "application/json", buf);
    free(buf);
}

/* ═══════════════════════════════════
   API: GET /api/garden — Garden plants with health data
═══════════════════════════════════ */
void api_garden(SOCKET fd, const char *body, MYSQL *db) {
    auto_kill(db);
    if (!db) { send_resp(fd, "500 Error", "application/json", "[]"); return; }
    int uid = validate_session(fd, db, body, 1);
    if (uid == -1) return;

    char sql[1024];
    snprintf(sql, sizeof(sql),
        "SELECT garden_id, plant_name, "
        "DATE_FORMAT(planted_on,'%%Y-%%m-%%d %%H:%%i:%%s'), "
        "DATE_FORMAT(last_watered,'%%Y-%%m-%%d %%H:%%i:%%s'), status, "
        "GREATEST(0, TIMESTAMPDIFF(MINUTE, NOW(), last_watered + INTERVAL 15 HOUR)) AS mins, "
        "is_favorite "
        "FROM my_garden WHERE user_id = %d ORDER BY garden_id", uid);

    if (mysql_query(db, sql)) {
        fprintf(stderr, "[ERROR] api_garden: %s\n", mysql_error(db));
        send_resp(fd, "500 Error", "application/json", "[]");
        return;
    }

    MYSQL_RES *res = mysql_store_result(db);
    if (!res) { send_resp(fd, "500 Error", "application/json", "[]"); return; }

    /* Dynamic buffer */
    int cap = 32768, len = 0;
    char *buf = (char *)malloc(cap);
    if (!buf) { mysql_free_result(res); send_resp(fd, "500 Error", "application/json", "[]"); return; }
    buf[len++] = '[';

    MYSQL_ROW row;
    int first = 1;
    while ((row = mysql_fetch_row(res))) {
        int dead = row[4] && strcmp(row[4], "DEAD") == 0;
        int fav  = row[6] ? atoi(row[6]) : 0;

        char esc_name[256] = {0};
        json_escape(row[1] ? row[1] : "", esc_name, sizeof(esc_name));

        char item[512];
        int n = snprintf(item, sizeof(item),
            "%s{\"id\":%s,\"name\":\"%s\",\"planted_on\":\"%s\","
            "\"last_watered\":\"%s\",\"dead\":%s,\"mins_left\":%s,\"fav\":%s}",
            first ? "" : ",",
            row[0]?row[0]:"0", esc_name,
            row[2]?row[2]:"", row[3]?row[3]:"",
            dead ? "true" : "false",
            row[5]?row[5]:"0",
            fav ? "true" : "false");
        first = 0;

        while (len + n + 2 >= cap) {
            cap *= 2;
            char *tmp = (char *)realloc(buf, cap);
            if (!tmp) { free(buf); mysql_free_result(res); send_resp(fd, "500 Error", "application/json", "[]"); return; }
            buf = tmp;
        }
        memcpy(buf + len, item, n);
        len += n;
    }
    mysql_free_result(res);
    buf[len++] = ']';
    buf[len] = '\0';
    send_resp(fd, "200 OK", "application/json", buf);
    free(buf);
}

/* ═══════════════════════════════════
   API: POST /api/garden/add
═══════════════════════════════════ */
void api_add(SOCKET fd, const char *body, MYSQL *db) {
    if (!db) { send_resp(fd, "500 Error", "application/json", "{\"ok\":false}"); return; }
    int uid = validate_session(fd, db, body, 0);
    if (uid == -1) return;

    char name[128] = {0};
    jget(body, "name", name, sizeof(name));
    if (!name[0]) {
        send_resp(fd, "400 Bad Request", "application/json", "{\"ok\":false,\"msg\":\"Plant name required\"}");
        return;
    }
    char safe[256];
    mysql_real_escape_string(db, safe, name, (unsigned long)strlen(name));
    char sql[512];
    snprintf(sql, sizeof(sql), "INSERT INTO my_garden(user_id, plant_name) VALUES(%d, '%s')", uid, safe);
    if (mysql_query(db, sql)) {
        fprintf(stderr, "[ERROR] api_add: %s\n", mysql_error(db));
        send_resp(fd, "500 Error", "application/json", "{\"ok\":false,\"msg\":\"Database error\"}");
        return;
    }
    /* Log activity */
    char detail[256];
    snprintf(detail, sizeof(detail), "Added %s to garden", name);
    api_log_activity(db, uid, "ADD", detail);

    send_resp(fd, "200 OK", "application/json", "{\"ok\":true}");
}

/* ═══════════════════════════════════
   API: POST /api/garden/water — Water a plant + record history
═══════════════════════════════════ */
void api_water(SOCKET fd, const char *body, MYSQL *db) {
    if (!db) { send_resp(fd, "500 Error", "application/json", "{\"ok\":false}"); return; }
    int uid = validate_session(fd, db, body, 0);
    if (uid == -1) return;

    char id[16] = {0};
    jget(body, "id", id, sizeof(id));
    if (!id[0]) {
        send_resp(fd, "400 Bad Request", "application/json", "{\"ok\":false}");
        return;
    }

    // Verify ownership
    char sql[512];
    snprintf(sql, sizeof(sql),
        "SELECT plant_name FROM my_garden WHERE garden_id=%s AND user_id=%d", id, uid);
    if (mysql_query(db, sql)) {
        send_resp(fd, "500 Error", "application/json", "{\"ok\":false}");
        return;
    }
    MYSQL_RES *res = mysql_store_result(db);
    if (!res) {
        send_resp(fd, "500 Error", "application/json", "{\"ok\":false}");
        return;
    }
    MYSQL_ROW row = mysql_fetch_row(res);
    if (!row || !row[0]) {
        mysql_free_result(res);
        send_resp(fd, "404 Not Found", "application/json", "{\"ok\":false,\"msg\":\"Plant not found in your garden\"}");
        return;
    }
    char plant_name[256];
    strncpy(plant_name, row[0], sizeof(plant_name) - 1);
    plant_name[sizeof(plant_name) - 1] = '\0';
    mysql_free_result(res);

    /* Update the plant */
    snprintf(sql, sizeof(sql),
        "UPDATE my_garden SET last_watered=NOW(), status='ALIVE' WHERE garden_id=%s AND user_id=%d", id, uid);
    mysql_query(db, sql);

    /* Record in water_history */
    snprintf(sql, sizeof(sql),
        "INSERT INTO water_history(garden_id, plant_name, watered_at) VALUES(%s, '%s', NOW())", id, plant_name);
    mysql_query(db, sql);

    /* Log activity */
    char detail[256];
    snprintf(detail, sizeof(detail), "Watered %s", plant_name);
    api_log_activity(db, uid, "WATER", detail);

    send_resp(fd, "200 OK", "application/json", "{\"ok\":true}");
}

/* ═══════════════════════════════════
   API: POST /api/garden/remove
═══════════════════════════════════ */
void api_remove(SOCKET fd, const char *body, MYSQL *db) {
    if (!db) { send_resp(fd, "500 Error", "application/json", "{\"ok\":false}"); return; }
    int uid = validate_session(fd, db, body, 0);
    if (uid == -1) return;

    char id[16] = {0};
    jget(body, "id", id, sizeof(id));
    if (!id[0]) {
        send_resp(fd, "400 Bad Request", "application/json", "{\"ok\":false}");
        return;
    }

    // Verify ownership and get name for activity log
    char sql[512];
    snprintf(sql, sizeof(sql), "SELECT plant_name FROM my_garden WHERE garden_id=%s AND user_id=%d", id, uid);
    if (mysql_query(db, sql)) {
        send_resp(fd, "500 Error", "application/json", "{\"ok\":false}");
        return;
    }
    MYSQL_RES *res = mysql_store_result(db);
    if (!res) {
        send_resp(fd, "500 Error", "application/json", "{\"ok\":false}");
        return;
    }
    MYSQL_ROW row = mysql_fetch_row(res);
    if (!row || !row[0]) {
        mysql_free_result(res);
        send_resp(fd, "404 Not Found", "application/json", "{\"ok\":false,\"msg\":\"Plant not found in your garden\"}");
        return;
    }
    char pname[128];
    strncpy(pname, row[0], sizeof(pname) - 1);
    pname[sizeof(pname) - 1] = '\0';
    mysql_free_result(res);

    snprintf(sql, sizeof(sql), "DELETE FROM my_garden WHERE garden_id=%s AND user_id=%d", id, uid);
    mysql_query(db, sql);

    char detail[256];
    snprintf(detail, sizeof(detail), "Removed %s from garden", pname);
    api_log_activity(db, uid, "REMOVE", detail);

    send_resp(fd, "200 OK", "application/json", "{\"ok\":true}");
}

/* ═══════════════════════════════════
   API: POST /api/garden/bulk-water — Water ALL alive plants
═══════════════════════════════════ */
void api_bulk_water(SOCKET fd, const char *body, MYSQL *db) {
    if (!db) { send_resp(fd, "500 Error", "application/json", "{\"ok\":false}"); return; }
    int uid = validate_session(fd, db, body, 0);
    if (uid == -1) return;

    char sql[512];
    /* Count alive plants first */
    snprintf(sql, sizeof(sql), "SELECT COUNT(*) FROM my_garden WHERE status='ALIVE' AND user_id=%d", uid);
    mysql_query(db, sql);
    MYSQL_RES *r = mysql_store_result(db);
    int count = 0;
    if (r) {
        MYSQL_ROW row = mysql_fetch_row(r);
        if (row && row[0]) count = atoi(row[0]);
        mysql_free_result(r);
    }

    /* Record history for each alive plant */
    snprintf(sql, sizeof(sql),
        "INSERT INTO water_history(garden_id, plant_name, watered_at) "
        "SELECT garden_id, plant_name, NOW() FROM my_garden WHERE status='ALIVE' AND user_id=%d", uid);
    mysql_query(db, sql);

    /* Update all alive plants */
    snprintf(sql, sizeof(sql),
        "UPDATE my_garden SET last_watered=NOW() WHERE status='ALIVE' AND user_id=%d", uid);
    mysql_query(db, sql);

    char detail[128];
    snprintf(detail, sizeof(detail), "Bulk watered %d plants", count);
    api_log_activity(db, uid, "BULK_WATER", detail);

    char resp[128];
    snprintf(resp, sizeof(resp), "{\"ok\":true,\"count\":%d}", count);
    send_resp(fd, "200 OK", "application/json", resp);
}

/* ═══════════════════════════════════
   API: POST /api/garden/fav — Toggle favourite status
═══════════════════════════════════ */
void api_toggle_fav(SOCKET fd, const char *body, MYSQL *db) {
    if (!db) { send_resp(fd, "500 Error", "application/json", "{\"ok\":false}"); return; }
    int uid = validate_session(fd, db, body, 0);
    if (uid == -1) return;

    char id[16] = {0};
    jget(body, "id", id, sizeof(id));
    if (!id[0]) {
        send_resp(fd, "400 Bad Request", "application/json", "{\"ok\":false}");
        return;
    }
    char sql[512];
    snprintf(sql, sizeof(sql),
        "UPDATE my_garden SET is_favorite = 1 - is_favorite WHERE garden_id=%s AND user_id=%d", id, uid);
    mysql_query(db, sql);
    send_resp(fd, "200 OK", "application/json", "{\"ok\":true}");
}

/* ═══════════════════════════════════
   API: POST /api/auth/login — User authentication
═══════════════════════════════════ */
void api_login(SOCKET fd, const char *body, MYSQL *db) {
    char user[64] = {0}, pass[64] = {0};
    jget(body, "username", user, sizeof(user));
    jget(body, "password", pass, sizeof(pass));

    if (!user[0] || !pass[0]) {
        send_resp(fd, "400 Bad Request", "application/json",
            "{\"ok\":false,\"msg\":\"Username and password required\"}");
        return;
    }

    char safe_user[128], safe_pass[128];
    mysql_real_escape_string(db, safe_user, user, (unsigned long)strlen(user));
    mysql_real_escape_string(db, safe_pass, pass, (unsigned long)strlen(pass));

    char sql[512];
    snprintf(sql, sizeof(sql),
        "SELECT user_id, display_name FROM users WHERE username='%s' AND password='%s'",
        safe_user, safe_pass);

    if (mysql_query(db, sql)) {
        send_resp(fd, "500 Error", "application/json", "{\"ok\":false,\"msg\":\"Database error\"}");
        return;
    }

    MYSQL_RES *res = mysql_store_result(db);
    MYSQL_ROW row = mysql_fetch_row(res);

    if (!row) {
        mysql_free_result(res);
        send_resp(fd, "401 Unauthorized", "application/json",
            "{\"ok\":false,\"msg\":\"Invalid username or password\"}");
        return;
    }

    int user_id = atoi(row[0]);
    char display[128] = {0};
    if (row[1]) strncpy(display, row[1], sizeof(display) - 1);
    mysql_free_result(res);

    /* Generate session */
    char session_id[33];
    generate_session_id(session_id, sizeof(session_id));

    snprintf(sql, sizeof(sql),
        "INSERT INTO sessions(session_id, user_id, expires_at) "
        "VALUES('%s', %d, NOW() + INTERVAL %d HOUR)",
        session_id, user_id, SESSION_HOURS);
    mysql_query(db, sql);

    /* Update last login */
    snprintf(sql, sizeof(sql),
        "UPDATE users SET last_login=NOW() WHERE user_id=%d", user_id);
    mysql_query(db, sql);

    /* Log activity */
    char detail[128];
    snprintf(detail, sizeof(detail), "%s logged in", user);
    api_log_activity(db, user_id, "LOGIN", detail);

    char esc_display[256] = {0};
    json_escape(display, esc_display, sizeof(esc_display));

    char resp[512];
    snprintf(resp, sizeof(resp),
        "{\"ok\":true,\"session\":\"%s\",\"username\":\"%s\",\"display_name\":\"%s\"}",
        session_id, safe_user, esc_display);
    send_resp(fd, "200 OK", "application/json", resp);
}

/* ═══════════════════════════════════
   API: POST /api/auth/register — Create new user
═══════════════════════════════════ */
void api_register(SOCKET fd, const char *body, MYSQL *db) {
    char user[64] = {0}, pass[64] = {0}, display[128] = {0};
    jget(body, "username", user, sizeof(user));
    jget(body, "password", pass, sizeof(pass));
    jget(body, "display_name", display, sizeof(display));

    if (!user[0] || !pass[0]) {
        send_resp(fd, "400 Bad Request", "application/json",
            "{\"ok\":false,\"msg\":\"Username and password required\"}");
        return;
    }

    if (strlen(pass) < 4) {
        send_resp(fd, "400 Bad Request", "application/json",
            "{\"ok\":false,\"msg\":\"Password must be at least 4 characters\"}");
        return;
    }

    if (!display[0]) strncpy(display, user, sizeof(display) - 1);

    char safe_user[128], safe_pass[128], safe_display[256];
    mysql_real_escape_string(db, safe_user, user, (unsigned long)strlen(user));
    mysql_real_escape_string(db, safe_pass, pass, (unsigned long)strlen(pass));
    mysql_real_escape_string(db, safe_display, display, (unsigned long)strlen(display));

    char sql[1024];
    snprintf(sql, sizeof(sql),
        "INSERT INTO users(username, password, display_name) VALUES('%s','%s','%s')",
        safe_user, safe_pass, safe_display);

    if (mysql_query(db, sql)) {
        const char *err = mysql_error(db);
        if (strstr(err, "Duplicate")) {
            send_resp(fd, "409 Conflict", "application/json",
                "{\"ok\":false,\"msg\":\"Username already taken\"}");
        } else {
            send_resp(fd, "500 Error", "application/json",
                "{\"ok\":false,\"msg\":\"Registration failed\"}");
        }
        return;
    }

    char detail[128];
    snprintf(detail, sizeof(detail), "New user registered: %s", user);
    api_log_activity(db, 0, "REGISTER", detail);

    send_resp(fd, "200 OK", "application/json", "{\"ok\":true,\"msg\":\"Account created successfully\"}");
}

/* ═══════════════════════════════════
   API: GET /api/water-history?id=N — Watering history for a plant
═══════════════════════════════════ */
void api_water_history(SOCKET fd, const char *body, MYSQL *db) {
    if (!db) { send_resp(fd, "500 Error", "application/json", "[]"); return; }
    /* body actually contains the full request for query params */
    int uid = validate_session(fd, db, body, 1);
    if (uid == -1) return;

    char id[16] = {0};
    get_query_param(body, "id", id, sizeof(id));
    if (!id[0]) {
        send_resp(fd, "400 Bad Request", "application/json", "[]");
        return;
    }

    char sql[512];
    snprintf(sql, sizeof(sql),
        "SELECT w.history_id, w.plant_name, DATE_FORMAT(w.watered_at, '%%Y-%%m-%%d %%H:%%i:%%s') "
        "FROM water_history w "
        "JOIN my_garden g ON w.garden_id = g.garden_id "
        "WHERE w.garden_id=%s AND g.user_id=%d ORDER BY w.watered_at DESC LIMIT 50", id, uid);

    if (mysql_query(db, sql)) {
        send_resp(fd, "500 Error", "application/json", "[]");
        return;
    }

    MYSQL_RES *res = mysql_store_result(db);
    if (!res) { send_resp(fd, "500 Error", "application/json", "[]"); return; }

    char buf[8192];
    int len = 0;
    buf[len++] = '[';
    MYSQL_ROW row;
    int first = 1;
    while ((row = mysql_fetch_row(res))) {
        char esc_name[256] = {0};
        json_escape(row[1]?row[1]:"", esc_name, sizeof(esc_name));
        len += snprintf(buf + len, sizeof(buf) - len,
            "%s{\"id\":%s,\"name\":\"%s\",\"watered_at\":\"%s\"}",
            first ? "" : ",",
            row[0]?row[0]:"0", esc_name, row[2]?row[2]:"");
        first = 0;
        if (len >= (int)sizeof(buf) - 200) break;
    }
    mysql_free_result(res);
    buf[len++] = ']';
    buf[len] = '\0';
    send_resp(fd, "200 OK", "application/json", buf);
}

/* ═══════════════════════════════════
   API: GET /api/stats — Aggregate garden statistics
═══════════════════════════════════ */
void api_stats(SOCKET fd, const char *body, MYSQL *db) {
    if (!db) { send_resp(fd, "500 Error", "application/json", "{}"); return; }
    int uid = validate_session(fd, db, body, 1);
    if (uid == -1) return;

    char buf[4096];
    int len = 0;
    len += snprintf(buf + len, sizeof(buf) - len, "{");

    char sql[1024];

    /* Total plants in garden by status */
    snprintf(sql, sizeof(sql), "SELECT status, COUNT(*) FROM my_garden WHERE user_id=%d GROUP BY status", uid);
    mysql_query(db, sql);
    MYSQL_RES *r = mysql_store_result(db);
    int alive = 0, dead = 0;
    if (r) {
        MYSQL_ROW row;
        while ((row = mysql_fetch_row(r))) {
            if (row[0] && strcmp(row[0], "ALIVE") == 0) alive = atoi(row[1]);
            else if (row[0] && strcmp(row[0], "DEAD") == 0) dead = atoi(row[1]);
        }
        mysql_free_result(r);
    }
    len += snprintf(buf + len, sizeof(buf) - len,
        "\"alive\":%d,\"dead\":%d,\"total\":%d", alive, dead, alive + dead);

    /* Total waterings */
    snprintf(sql, sizeof(sql), 
        "SELECT COUNT(*) FROM water_history w "
        "JOIN my_garden g ON w.garden_id = g.garden_id "
        "WHERE g.user_id=%d", uid);
    mysql_query(db, sql);
    r = mysql_store_result(db);
    if (r) {
        MYSQL_ROW row = mysql_fetch_row(r);
        len += snprintf(buf + len, sizeof(buf) - len,
            ",\"total_waterings\":%s", row && row[0] ? row[0] : "0");
        mysql_free_result(r);
    }

    /* Waterings today */
    snprintf(sql, sizeof(sql), 
        "SELECT COUNT(*) FROM water_history w "
        "JOIN my_garden g ON w.garden_id = g.garden_id "
        "WHERE DATE(w.watered_at)=CURDATE() AND g.user_id=%d", uid);
    mysql_query(db, sql);
    r = mysql_store_result(db);
    if (r) {
        MYSQL_ROW row = mysql_fetch_row(r);
        len += snprintf(buf + len, sizeof(buf) - len,
            ",\"waterings_today\":%s", row && row[0] ? row[0] : "0");
        mysql_free_result(r);
    }

    /* Waterings this week */
    snprintf(sql, sizeof(sql), 
        "SELECT COUNT(*) FROM water_history w "
        "JOIN my_garden g ON w.garden_id = g.garden_id "
        "WHERE w.watered_at >= DATE_SUB(NOW(), INTERVAL 7 DAY) AND g.user_id=%d", uid);
    mysql_query(db, sql);
    r = mysql_store_result(db);
    if (r) {
        MYSQL_ROW row = mysql_fetch_row(r);
        len += snprintf(buf + len, sizeof(buf) - len,
            ",\"waterings_week\":%s", row && row[0] ? row[0] : "0");
        mysql_free_result(r);
    }

    /* Plant type distribution */
    snprintf(sql, sizeof(sql),
        "SELECT p.plant_type, COUNT(*) FROM my_garden g "
        "JOIN plants p ON g.plant_name=p.plant_name "
        "WHERE g.status='ALIVE' AND g.user_id=%d GROUP BY p.plant_type ORDER BY COUNT(*) DESC", uid);
    mysql_query(db, sql);
    r = mysql_store_result(db);
    if (r) {
        len += snprintf(buf + len, sizeof(buf) - len, ",\"type_dist\":[");
        MYSQL_ROW row;
        int first = 1;
        while ((row = mysql_fetch_row(r))) {
            char esc[128] = {0};
            json_escape(row[0] ? row[0] : "Other", esc, sizeof(esc));
            len += snprintf(buf + len, sizeof(buf) - len,
                "%s{\"type\":\"%s\",\"count\":%s}",
                first ? "" : ",", esc, row[1] ? row[1] : "0");
            first = 0;
        }
        len += snprintf(buf + len, sizeof(buf) - len, "]");
        mysql_free_result(r);
    }

    /* Most watered plants (top 5) */
    snprintf(sql, sizeof(sql),
        "SELECT w.plant_name, COUNT(*) as cnt FROM water_history w "
        "JOIN my_garden g ON w.garden_id = g.garden_id "
        "WHERE g.user_id=%d "
        "GROUP BY w.plant_name ORDER BY cnt DESC LIMIT 5", uid);
    mysql_query(db, sql);
    r = mysql_store_result(db);
    if (r) {
        len += snprintf(buf + len, sizeof(buf) - len, ",\"top_watered\":[");
        MYSQL_ROW row;
        int first = 1;
        while ((row = mysql_fetch_row(r))) {
            char esc[256] = {0};
            json_escape(row[0] ? row[0] : "", esc, sizeof(esc));
            len += snprintf(buf + len, sizeof(buf) - len,
                "%s{\"name\":\"%s\",\"count\":%s}",
                first ? "" : ",", esc, row[1] ? row[1] : "0");
            first = 0;
        }
        len += snprintf(buf + len, sizeof(buf) - len, "]");
        mysql_free_result(r);
    }

    /* Waterings per day (last 7 days) for chart */
    snprintf(sql, sizeof(sql),
        "SELECT DATE_FORMAT(d.dt,'%%Y-%%m-%%d'), COALESCE(COUNT(h.history_id),0) "
        "FROM (SELECT CURDATE() - INTERVAL n DAY AS dt FROM "
        "(SELECT 0 n UNION SELECT 1 UNION SELECT 2 UNION SELECT 3 "
        "UNION SELECT 4 UNION SELECT 5 UNION SELECT 6) nums) d "
        "LEFT JOIN (SELECT history_id, watered_at FROM water_history w "
        "           JOIN my_garden g ON w.garden_id = g.garden_id "
        "           WHERE g.user_id=%d) h ON DATE(h.watered_at)=d.dt "
        "GROUP BY d.dt ORDER BY d.dt", uid);
    mysql_query(db, sql);
    r = mysql_store_result(db);
    if (r) {
        len += snprintf(buf + len, sizeof(buf) - len, ",\"daily_waterings\":[");
        MYSQL_ROW row;
        int first = 1;
        while ((row = mysql_fetch_row(r))) {
            len += snprintf(buf + len, sizeof(buf) - len,
                "%s{\"date\":\"%s\",\"count\":%s}",
                first ? "" : ",", row[0] ? row[0] : "", row[1] ? row[1] : "0");
            first = 0;
        }
        len += snprintf(buf + len, sizeof(buf) - len, "]");
        mysql_free_result(r);
    }

    len += snprintf(buf + len, sizeof(buf) - len, "}");
    send_resp(fd, "200 OK", "application/json", buf);
}

/* ═══════════════════════════════════
   API: POST /api/notes/add — Add a note to a garden plant
 ═══════════════════════════════════ */
void api_add_note(SOCKET fd, const char *body, MYSQL *db) {
    if (!db) { send_resp(fd, "500 Error", "application/json", "{\"ok\":false}"); return; }
    int uid = validate_session(fd, db, body, 0);
    if (uid == -1) return;

    char id[16] = {0}, note[512] = {0};
    jget(body, "id", id, sizeof(id));
    jget(body, "note", note, sizeof(note));

    if (!id[0] || !note[0]) {
        send_resp(fd, "400 Bad Request", "application/json", "{\"ok\":false}");
        return;
    }

    // Verify ownership of the garden plant
    char sql[1536];
    snprintf(sql, sizeof(sql), "SELECT 1 FROM my_garden WHERE garden_id=%s AND user_id=%d", id, uid);
    if (mysql_query(db, sql)) {
        send_resp(fd, "500 Error", "application/json", "{\"ok\":false}");
        return;
    }
    MYSQL_RES *res = mysql_store_result(db);
    if (!res) {
        send_resp(fd, "500 Error", "application/json", "{\"ok\":false}");
        return;
    }
    MYSQL_ROW row = mysql_fetch_row(res);
    if (!row) {
        mysql_free_result(res);
        send_resp(fd, "404 Not Found", "application/json", "{\"ok\":false,\"msg\":\"Plant not found in your garden\"}");
        return;
    }
    mysql_free_result(res);

    char safe_note[1024];
    mysql_real_escape_string(db, safe_note, note, (unsigned long)strlen(note));

    snprintf(sql, sizeof(sql),
        "INSERT INTO plant_notes(garden_id, note_text) VALUES(%s, '%s')", id, safe_note);

    if (mysql_query(db, sql)) {
        send_resp(fd, "500 Error", "application/json", "{\"ok\":false}");
        return;
    }
    send_resp(fd, "200 OK", "application/json", "{\"ok\":true}");
}

/* ═══════════════════════════════════
   API: GET /api/notes?id=N — Get notes for a plant
═══════════════════════════════════ */
void api_get_notes(SOCKET fd, const char *body, MYSQL *db) {
    if (!db) { send_resp(fd, "200 OK", "application/json", "[]"); return; }
    int uid = validate_session(fd, db, body, 1);
    if (uid == -1) return;

    char id[16] = {0};
    get_query_param(body, "id", id, sizeof(id));
    if (!id[0]) {
        send_resp(fd, "200 OK", "application/json", "[]");
        return;
    }

    char sql[512];
    snprintf(sql, sizeof(sql),
        "SELECT n.note_id, n.note_text, DATE_FORMAT(n.created_at, '%%Y-%%m-%%d %%H:%%i') "
        "FROM plant_notes n "
        "JOIN my_garden g ON n.garden_id = g.garden_id "
        "WHERE n.garden_id=%s AND g.user_id=%d ORDER BY n.created_at DESC LIMIT 20", id, uid);

    if (mysql_query(db, sql)) {
        send_resp(fd, "200 OK", "application/json", "[]");
        return;
    }

    MYSQL_RES *res = mysql_store_result(db);
    if (!res) { send_resp(fd, "200 OK", "application/json", "[]"); return; }

    char buf[8192];
    int len = 0;
    buf[len++] = '[';
    MYSQL_ROW row;
    int first = 1;
    while ((row = mysql_fetch_row(res))) {
        char esc[1024] = {0};
        json_escape(row[1] ? row[1] : "", esc, sizeof(esc));
        len += snprintf(buf + len, sizeof(buf) - len,
            "%s{\"id\":%s,\"note\":\"%s\",\"created\":\"%s\"}",
            first ? "" : ",",
            row[0]?row[0]:"0", esc, row[2]?row[2]:"");
        first = 0;
        if (len >= (int)sizeof(buf) - 200) break;
    }
    mysql_free_result(res);
    buf[len++] = ']';
    buf[len] = '\0';
    send_resp(fd, "200 OK", "application/json", buf);
}

/* ═══════════════════════════════════
   ACTIVITY LOG — Insert and retrieve
═══════════════════════════════════ */
void api_log_activity(MYSQL *db, int user_id, const char *action, const char *details) {
    if (!db) return;
    char safe_action[128], safe_details[512];
    mysql_real_escape_string(db, safe_action, action, (unsigned long)strlen(action));
    mysql_real_escape_string(db, safe_details, details, (unsigned long)strlen(details));

    char sql[768];
    if (user_id > 0) {
        snprintf(sql, sizeof(sql),
            "INSERT INTO activity_log(user_id, action, details) VALUES(%d, '%s', '%s')",
            user_id, safe_action, safe_details);
    } else {
        snprintf(sql, sizeof(sql),
            "INSERT INTO activity_log(user_id, action, details) VALUES(NULL, '%s', '%s')",
            safe_action, safe_details);
    }
    mysql_query(db, sql);
}

void api_get_activity(SOCKET fd, const char *body, MYSQL *db) {
    if (!db) { send_resp(fd, "200 OK", "application/json", "[]"); return; }
    int uid = validate_session(fd, db, body, 1);
    if (uid == -1) return;

    char sql[512];
    snprintf(sql, sizeof(sql),
        "SELECT log_id, action, details, DATE_FORMAT(created_at, '%%Y-%%m-%%d %%H:%%i:%%s') "
        "FROM activity_log WHERE user_id = %d ORDER BY created_at DESC LIMIT 20", uid);

    if (mysql_query(db, sql)) {
        send_resp(fd, "200 OK", "application/json", "[]");
        return;
    }

    MYSQL_RES *res = mysql_store_result(db);
    if (!res) { send_resp(fd, "200 OK", "application/json", "[]"); return; }

    char buf[8192];
    int len = 0;
    buf[len++] = '[';
    MYSQL_ROW row;
    int first = 1;
    while ((row = mysql_fetch_row(res))) {
        char esc_action[128] = {0}, esc_detail[512] = {0};
        json_escape(row[1] ? row[1] : "", esc_action, sizeof(esc_action));
        json_escape(row[2] ? row[2] : "", esc_detail, sizeof(esc_detail));
        len += snprintf(buf + len, sizeof(buf) - len,
            "%s{\"id\":%s,\"action\":\"%s\",\"details\":\"%s\",\"time\":\"%s\"}",
            first ? "" : ",",
            row[0]?row[0]:"0", esc_action, esc_detail, row[3]?row[3]:"");
        first = 0;
        if (len >= (int)sizeof(buf) - 300) break;
    }
    mysql_free_result(res);
    buf[len++] = ']';
    buf[len] = '\0';
    send_resp(fd, "200 OK", "application/json", buf);
}

/* ═══════════════════════════════════
   API: GET /api/server-info — Server uptime & stats
═══════════════════════════════════ */
void api_server_info(SOCKET fd, const char *body, MYSQL *db) {
    (void)body; (void)db;
    time_t now = time(NULL);
    long uptime = (long)(now - g_start_time);
    int hours = (int)(uptime / 3600);
    int mins  = (int)((uptime % 3600) / 60);
    int secs  = (int)(uptime % 60);

    char resp[256];
    snprintf(resp, sizeof(resp),
        "{\"uptime\":\"%dh %dm %ds\",\"uptime_secs\":%ld,\"total_requests\":%ld}",
        hours, mins, secs, uptime, g_total_requests);
    send_resp(fd, "200 OK", "application/json", resp);
}
