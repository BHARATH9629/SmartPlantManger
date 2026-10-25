#ifndef HTTP_H
#define HTTP_H

/* ============================================================
   http.h — HTTP Helper Function Declarations
   Handles: response building, JSON parsing, query params
   ============================================================ */

#include "config.h"

/* Send an HTTP response with headers */
void send_resp(SOCKET fd, const char *status, const char *ctype, const char *body);

/* Extract POST body from raw HTTP request (after \r\n\r\n) */
void post_body(const char *req, char *out, int maxlen);

/* Extract a JSON string value by key (minimal parser) */
void jget(const char *json, const char *key, char *out, int maxlen);

/* Escape special chars for safe JSON embedding */
void json_escape(const char *in, char *out, int maxlen);

/* Extract a query parameter from the request URL */
void get_query_param(const char *req, const char *key, char *out, int maxlen);

/* URL-decode a string (handles %20, +, etc.) */
void url_decode(const char *src, char *dst, int maxlen);

#endif /* HTTP_H */
