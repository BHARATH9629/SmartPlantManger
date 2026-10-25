#ifndef AI_H
#define AI_H

/* ============================================================
   ai.h — AI Chatbot Proxy Declarations
   Proxies requests to Groq API via WinHTTP
   ============================================================ */

#include "config.h"

/* Handle POST /api/chat — proxy to Groq LLM API */
void api_chat(SOCKET fd, const char *body, MYSQL *db);

#endif /* AI_H */
