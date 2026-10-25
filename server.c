/* ============================================================
   server.c — Smart Plant Manager v2 — Multi-threaded HTTP Server
   
   Demonstrates:
   - Multi-threading with _beginthreadex() and CRITICAL_SECTION
   - Function pointer dispatch table for routing
   - File I/O for request logging
   - Graceful shutdown with SetConsoleCtrlHandler
   - Atomic operations with InterlockedIncrement
   
   Compile (MinGW-w64):
     gcc server.c http.c db.c ai.c
         -I"C:\Program Files\MySQL\MySQL Server 8.0\include"
         -L"C:\Program Files\MySQL\MySQL Server 8.0\lib"
         -lmysql -lws2_32 -lwinhttp -o server.exe
   ============================================================ */

#include "config.h"
#include "http.h"
#include "db.h"
#include "ai.h"

/* ═══════════════════════════════════
   GLOBAL STATE
   Shared across threads — protected by mutexes
═══════════════════════════════════ */
static MYSQL             *g_db          = NULL;
static CRITICAL_SECTION   g_db_lock;       /* Mutex for MySQL access   */
static CRITICAL_SECTION   g_log_lock;      /* Mutex for log file       */
static FILE              *g_logfile     = NULL;
volatile long             g_total_requests = 0;   /* Atomic counter    */
time_t                    g_start_time;            /* Server start time */
static volatile long      g_active_conns   = 0;   /* Active threads    */
static volatile int       g_running        = 1;    /* Shutdown flag     */

/* ═══════════════════════════════════
   REQUEST LOGGING — File I/O with rotation
   Writes to server.log with timestamp and client info
═══════════════════════════════════ */
static void log_request(const char *client_ip, const char *method, const char *path, int status) {
    if (!g_logfile) return;

    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char timebuf[32];
    strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S", t);

    EnterCriticalSection(&g_log_lock);

    /* Check file size for rotation */
    long pos = ftell(g_logfile);
    if (pos > LOG_MAX_SIZE) {
        fclose(g_logfile);
        remove("server.log.old");
        rename(LOG_FILE, "server.log.old");
        g_logfile = fopen(LOG_FILE, "a");
    }

    if (g_logfile) {
        fprintf(g_logfile, "[%s] %s %s %s -> %d\n", timebuf, client_ip, method, path, status);
        fflush(g_logfile);
    }

    LeaveCriticalSection(&g_log_lock);

    /* Also print to console with color */
    printf("  %s %s %-30s -> %d\n", client_ip, method, path, status);
}

/* ═══════════════════════════════════
   EXTRACT METHOD & PATH from raw HTTP request line
   "GET /api/plants HTTP/1.1\r\n..." -> method="GET", path="/api/plants"
═══════════════════════════════════ */
static void parse_request_line(const char *req, char *method, int mlen, char *path, int plen) {
    int i = 0;
    /* Method */
    while (req[i] && req[i] != ' ' && i < mlen - 1) {
        method[i] = req[i]; i++;
    }
    method[i] = '\0';
    if (req[i] == ' ') i++;

    /* Path (with query string) */
    int j = 0;
    while (req[i] && req[i] != ' ' && req[i] != '\r' && j < plen - 1) {
        path[j++] = req[i++];
    }
    path[j] = '\0';
}

/* ═══════════════════════════════════
   SERVE index.html — Read file from disk
   Demonstrates: fopen, fseek, ftell, fread, malloc, free
═══════════════════════════════════ */
static void serve_html(SOCKET fd, const char *body, MYSQL *db) {
    (void)body; (void)db;
    FILE *f = fopen("index.html", "rb");
    if (!f) {
        send_resp(fd, "404 Not Found", "text/plain", "index.html not found");
        return;
    }
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    rewind(f);

    char *html = (char *)malloc(sz + 1);
    if (!html) {
        fclose(f);
        send_resp(fd, "500 Error", "text/plain", "Out of memory");
        return;
    }
    fread(html, 1, sz, f);
    html[sz] = '\0';
    fclose(f);

    send_resp(fd, "200 OK", "text/html; charset=utf-8", html);
    free(html);
}

/* ═══════════════════════════════════
   ROUTE TABLE — Function pointer dispatch
   Maps HTTP method + path to handler functions
   Demonstrates: arrays of structs with function pointers
═══════════════════════════════════ */
static Route routes[] = {
    /* Static files */
    { "GET",  "/",                     serve_html       },
    { "GET",  "/index.html",           serve_html       },

    /* Plant encyclopedia */
    { "GET",  "/api/plants",           api_plants       },

    /* Garden CRUD */
    { "GET",  "/api/garden",           api_garden       },
    { "POST", "/api/garden/add",       api_add          },
    { "POST", "/api/garden/water",     api_water        },
    { "POST", "/api/garden/remove",    api_remove       },
    { "POST", "/api/garden/bulk-water",api_bulk_water   },
    { "POST", "/api/garden/fav",       api_toggle_fav   },

    /* Authentication */
    { "POST", "/api/auth/login",       api_login        },
    { "POST", "/api/auth/register",    api_register     },

    /* Water history & stats */
    { "GET",  "/api/water-history",    api_water_history},
    { "GET",  "/api/stats",            api_stats        },

    /* Notes */
    { "POST", "/api/notes/add",        api_add_note     },
    { "GET",  "/api/notes",            api_get_notes    },

    /* Activity log */
    { "GET",  "/api/activity",         api_get_activity },

    /* Server info */
    { "GET",  "/api/server-info",      api_server_info  },

    /* AI chatbot */
    { "POST", "/api/chat",             api_chat         },

    { NULL, NULL, NULL }  /* sentinel */
};

#define NUM_ROUTES (sizeof(routes) / sizeof(routes[0]) - 1)

/* ═══════════════════════════════════
   FIND ROUTE — Match request to handler using function pointers
═══════════════════════════════════ */
static RouteHandler find_route(const char *method, const char *path) {
    /* Extract base path (without query string) for matching */
    char base[256];
    strncpy(base, path, sizeof(base) - 1);
    base[sizeof(base) - 1] = '\0';
    char *q = strchr(base, '?');
    if (q) *q = '\0';

    for (int i = 0; routes[i].method != NULL; i++) {
        if (strcmp(method, routes[i].method) == 0 &&
            strcmp(base, routes[i].path) == 0) {
            return routes[i].handler;
        }
    }
    return NULL;
}

/* ═══════════════════════════════════
   CLIENT THREAD — Handles one HTTP connection
   Each accepted connection runs in its own thread
   Demonstrates: _beginthreadex, CRITICAL_SECTION locking
═══════════════════════════════════ */
static unsigned __stdcall handle_client(void *arg) {
    SOCKET cfd = (SOCKET)(uintptr_t)arg;

    /* Read the HTTP request */
    char *req = (char *)malloc(BUFSZ);
    if (!req) {
        CLOSESOCK(cfd);
        InterlockedDecrement(&g_active_conns);
        return 0;
    }
    memset(req, 0, BUFSZ);
    recv(cfd, req, BUFSZ - 1, 0);

    /* Parse method and path */
    char method[16] = {0}, path[512] = {0};
    parse_request_line(req, method, sizeof(method), path, sizeof(path));

    int status = 200;

    /* Handle CORS preflight */
    if (strcmp(method, "OPTIONS") == 0) {
        send_resp(cfd, "204 No Content", "text/plain", "");
        log_request("127.0.0.1", method, path, 204);
        free(req);
        CLOSESOCK(cfd);
        InterlockedDecrement(&g_active_conns);
        return 0;
    }

    /* Find matching route handler */
    RouteHandler handler = find_route(method, path);

    if (handler) {
        /* Extract POST body */
        char body[4096] = {0};
        if (strcmp(method, "POST") == 0) {
            post_body(req, body, sizeof(body));
        }

        /*
         * CRITICAL SECTION — Thread-safe MySQL access
         * Only one thread can use the MySQL connection at a time.
         * This prevents data corruption from concurrent queries.
         */
        EnterCriticalSection(&g_db_lock);

        /* For GET endpoints that need query params or session info, pass full request */
        if (strcmp(method, "GET") == 0) {
            handler(cfd, req, g_db);
        } else {
            handler(cfd, body, g_db);
        }

        LeaveCriticalSection(&g_db_lock);
        status = 200;
    } else {
        send_resp(cfd, "404 Not Found", "application/json",
            "{\"error\":\"Route not found\"}");
        status = 404;
    }

    log_request("127.0.0.1", method, path, status);

    free(req);
    CLOSESOCK(cfd);
    InterlockedDecrement(&g_active_conns);
    return 0;
}

/* ═══════════════════════════════════
   GRACEFUL SHUTDOWN — Handle Ctrl+C
   Demonstrates: SetConsoleCtrlHandler, cleanup
═══════════════════════════════════ */
static BOOL WINAPI ctrl_handler(DWORD sig) {
    if (sig == CTRL_C_EVENT || sig == CTRL_CLOSE_EVENT) {
        printf("\n[SHUTDOWN] Cleaning up...\n");
        g_running = 0;

        /* Close log file */
        if (g_logfile) {
            fclose(g_logfile);
            g_logfile = NULL;
        }

        /* Close database */
        if (g_db) {
            mysql_close(g_db);
            g_db = NULL;
        }

        /* Cleanup critical sections */
        DeleteCriticalSection(&g_db_lock);
        DeleteCriticalSection(&g_log_lock);

        WSACleanup();
        printf("[SHUTDOWN] Server stopped.\n");
        ExitProcess(0);
    }
    return TRUE;
}

/* ═══════════════════════════════════
   MAIN — Server entry point
═══════════════════════════════════ */
int main(void) {
    /* Load environment configuration from .env file */
    load_env(".env");

    /* Seed random number generator */
    srand((unsigned)(time(NULL) ^ GetCurrentProcessId()));

    /* Record start time for uptime tracking */
    g_start_time = time(NULL);

    /* Initialize Winsock */
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        fprintf(stderr, "[ERROR] WSAStartup failed\n");
        return 1;
    }

    /* Initialize mutexes */
    InitializeCriticalSection(&g_db_lock);
    InitializeCriticalSection(&g_log_lock);

    /* Register Ctrl+C handler */
    SetConsoleCtrlHandler(ctrl_handler, TRUE);

    /* Open log file */
    g_logfile = fopen(LOG_FILE, "a");
    if (!g_logfile) {
        fprintf(stderr, "[WARN] Cannot open %s for logging\n", LOG_FILE);
    }

    /* Connect to MySQL */
    if (!db_connect(&g_db)) {
        fprintf(stderr, "[ERROR] Cannot connect to MySQL database '%s'\n", DB_NAME);
        fprintf(stderr, "        Make sure you have run: mysql -u root -p < setup_v2.sql\n");
        return 1;
    }

    printf("\n");
    printf("  ╔══════════════════════════════════════════════╗\n");
    printf("  ║   🌿 Smart Plant Manager v2 — HTTP Server   ║\n");
    printf("  ╠══════════════════════════════════════════════╣\n");
    printf("  ║  Database : %-32s ║\n", DB_NAME);
    printf("  ║  Port     : %-32d ║\n", PORT);
    printf("  ║  Threads  : Multi-threaded (max %d)         ║\n", MAX_CONNECTIONS);
    printf("  ║  Routes   : %-32d ║\n", (int)NUM_ROUTES);
    printf("  ║  Log file : %-32s ║\n", LOG_FILE);
    printf("  ╠══════════════════════════════════════════════╣\n");
    printf("  ║  Open -> http://localhost:%d              ║\n", PORT);
    printf("  ║  Press Ctrl+C to stop the server             ║\n");
    printf("  ╚══════════════════════════════════════════════╝\n\n");

    /* Print registered routes */
    printf("  Registered routes:\n");
    for (int i = 0; routes[i].method != NULL; i++) {
        printf("    %-6s %s\n", routes[i].method, routes[i].path);
    }
    printf("\n  Waiting for connections...\n\n");

    /* Create server socket */
    SOCKET srv = socket(AF_INET, SOCK_STREAM, 0);
    if (srv == INVALID_SOCKET) {
        fprintf(stderr, "[ERROR] socket() failed: %d\n", WSAGetLastError());
        return 1;
    }

    /* Allow address reuse */
    int opt = 1;
    setsockopt(srv, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt));

    /* Bind to port */
    struct sockaddr_in addr = {0};
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons(PORT);

    if (bind(srv, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        fprintf(stderr, "[ERROR] bind() failed on port %d: %d\n", PORT, WSAGetLastError());
        fprintf(stderr, "        Is another server already running on this port?\n");
        CLOSESOCK(srv);
        return 1;
    }

    /* Listen for connections */
    listen(srv, 32);

    /* ── Main accept loop ── */
    while (g_running) {
        struct sockaddr_in cli;
        socklen_t clen = sizeof(cli);
        SOCKET cfd = accept(srv, (struct sockaddr *)&cli, &clen);
        if (cfd == INVALID_SOCKET) continue;

        InterlockedIncrement(&g_total_requests);
        InterlockedIncrement(&g_active_conns);

        /* Check connection limit */
        if (g_active_conns > MAX_CONNECTIONS) {
            send_resp(cfd, "503 Service Unavailable", "application/json",
                "{\"error\":\"Too many connections\"}");
            CLOSESOCK(cfd);
            InterlockedDecrement(&g_active_conns);
            continue;
        }

        /*
         * SPAWN NEW THREAD for each connection
         * _beginthreadex is the C runtime thread creation function
         * Each thread independently handles one HTTP request
         */
        uintptr_t thread = _beginthreadex(
            NULL,                           /* security attrs */
            0,                              /* stack size (default) */
            handle_client,                  /* thread function */
            (void *)(uintptr_t)cfd,         /* pass socket as argument */
            0,                              /* run immediately */
            NULL                            /* thread ID (unused) */
        );

        if (thread == 0) {
            /* Thread creation failed — handle inline */
            fprintf(stderr, "[ERROR] _beginthreadex failed\n");
            CLOSESOCK(cfd);
            InterlockedDecrement(&g_active_conns);
        } else {
            /* Detach thread — we don't need to join */
            CloseHandle((HANDLE)thread);
        }
    }

    /* Cleanup (reached if g_running set to 0) */
    CLOSESOCK(srv);
    db_close(g_db);
    if (g_logfile) fclose(g_logfile);
    DeleteCriticalSection(&g_db_lock);
    DeleteCriticalSection(&g_log_lock);
    WSACleanup();

    return 0;
}