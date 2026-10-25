#ifndef CONFIG_H
#define CONFIG_H

/* ============================================================
   config.h — Smart Plant Manager v2 Configuration
   Central configuration file for all modules.
   Demonstrates: preprocessor macros, platform guards, typedefs
   ============================================================ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
  #include <winsock2.h>
  #include <ws2tcpip.h>
  #include <windows.h>
  #include <process.h>
  #pragma comment(lib,"ws2_32.lib")
  #define CLOSESOCK closesocket
  typedef int socklen_t;
#else
  #include <unistd.h>
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <arpa/inet.h>
  #include <pthread.h>
  #define CLOSESOCK close
  #define SOCKET int
  #define INVALID_SOCKET (-1)
#endif

#include <mysql.h>
#include "env.h"

/* ── Server Settings ── */
#define PORT            (get_server_port())
#define BUFSZ           262144   /* 256 KB request buffer */
#define MAX_CONNECTIONS 32       /* max concurrent threads */
#define SESSION_HOURS   24       /* session expiry in hours */

/* ── Database Credentials (Loaded from .env / environment) ── */
#define DB_HOST  (get_db_host())
#define DB_USER  (get_db_user())
#define DB_PASS  (get_db_pass())
#define DB_NAME  (get_db_name())

/* ── Groq AI Settings (Loaded from .env / environment) ── */
#define GROQ_KEY  (get_groq_key())
#define GROQ_HOST L"api.groq.com"

/* ── Log file ── */
#define LOG_FILE      "server.log"
#define LOG_MAX_SIZE  (1024 * 1024)  /* 1 MB rotation */

/* ── Route handler function pointer type ── */
typedef void (*RouteHandler)(SOCKET fd, const char *body, MYSQL *db);

/* Route entry for dispatch table */
typedef struct {
    const char   *method;   /* "GET" or "POST" */
    const char   *path;     /* e.g. "/api/plants" */
    RouteHandler  handler;
} Route;

#endif /* CONFIG_H */
