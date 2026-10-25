/* ============================================================
   http.c — HTTP Helper Functions Implementation
   Demonstrates: string manipulation, snprintf, pointer arithmetic
   ============================================================ */

#include "http.h"

/* ═══════════════════════════════════
   send_resp — Build and send a full HTTP/1.1 response
   Manually constructs status line, headers, and body
═══════════════════════════════════ */
void send_resp(SOCKET fd, const char *status, const char *ctype, const char *body) {
    size_t body_len = strlen(body);
    char hdr[1024];
    snprintf(hdr, sizeof(hdr),
        "HTTP/1.1 %s\r\n"
        "Content-Type: %s\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Access-Control-Allow-Methods: GET,POST,OPTIONS\r\n"
        "Access-Control-Allow-Headers: Content-Type,Authorization,X-Session\r\n"
        "Content-Length: %zu\r\n"
        "Connection: close\r\n\r\n",
        status, ctype, body_len);
    send(fd, hdr, (int)strlen(hdr), 0);
    /* Send body in chunks if very large */
    int sent = 0;
    while (sent < (int)body_len) {
        int chunk = (int)body_len - sent;
        if (chunk > 32768) chunk = 32768;
        int n = send(fd, body + sent, chunk, 0);
        if (n <= 0) break;
        sent += n;
    }
}

/* ═══════════════════════════════════
   post_body — Extract JSON body from raw HTTP POST
   Finds the \r\n\r\n delimiter between headers and body
═══════════════════════════════════ */
void post_body(const char *req, char *out, int maxlen) {
    const char *p = strstr(req, "\r\n\r\n");
    if (p) {
        strncpy(out, p + 4, maxlen - 1);
        out[maxlen - 1] = '\0';
    } else {
        out[0] = '\0';
    }
}

/* ═══════════════════════════════════
   jget — Minimal JSON key-value extractor
   Finds "key": "value" or "key": number in JSON string
   No external JSON library needed — pure C string parsing
═══════════════════════════════════ */
void jget(const char *json, const char *key, char *out, int maxlen) {
    char srch[128];
    snprintf(srch, sizeof(srch), "\"%s\":", key);
    const char *p = strstr(json, srch);
    if (!p) { out[0] = '\0'; return; }
    p += strlen(srch);
    while (*p == ' ') p++;
    int i = 0;
    if (*p == '"') {
        p++;
        while (*p && *p != '"' && i < maxlen - 1) {
            /* Handle escaped quotes inside JSON strings */
            if (*p == '\\' && *(p + 1) == '"') {
                out[i++] = '"';
                p += 2;
            } else {
                out[i++] = *p++;
            }
        }
    } else {
        while (*p && *p != ',' && *p != '}' && *p != ' ' && i < maxlen - 1) {
            out[i++] = *p++;
        }
    }
    out[i] = '\0';
}

/* ═══════════════════════════════════
   json_escape — Escape special characters for JSON
   Handles: " \ \n \r \t
═══════════════════════════════════ */
void json_escape(const char *in, char *out, int maxlen) {
    int j = 0;
    for (int i = 0; in[i] && j < maxlen - 2; i++) {
        switch (in[i]) {
            case '"':  out[j++] = '\\'; out[j++] = '"';  break;
            case '\\': out[j++] = '\\'; out[j++] = '\\'; break;
            case '\n': out[j++] = '\\'; out[j++] = 'n';  break;
            case '\r': out[j++] = '\\'; out[j++] = 'r';  break;
            case '\t': out[j++] = '\\'; out[j++] = 't';  break;
            default:   out[j++] = in[i]; break;
        }
    }
    out[j] = '\0';
}

/* ═══════════════════════════════════
   get_query_param — Extract URL query parameter
   e.g. from "GET /api/history?id=5 HTTP/1.1" extract id=5
═══════════════════════════════════ */
void get_query_param(const char *req, const char *key, char *out, int maxlen) {
    char search[64];
    snprintf(search, sizeof(search), "%s=", key);
    const char *p = strstr(req, search);
    if (!p) { out[0] = '\0'; return; }
    p += strlen(search);
    int i = 0;
    while (*p && *p != '&' && *p != ' ' && *p != '\r' && *p != '\n' && i < maxlen - 1) {
        out[i++] = *p++;
    }
    out[i] = '\0';
}

/* ═══════════════════════════════════
   url_decode — Decode percent-encoded URL strings
   %20 -> space, + -> space, %2F -> /, etc.
═══════════════════════════════════ */
void url_decode(const char *src, char *dst, int maxlen) {
    int i = 0;
    while (*src && i < maxlen - 1) {
        if (*src == '%' && src[1] && src[2]) {
            char hex[3] = { src[1], src[2], '\0' };
            dst[i++] = (char)strtol(hex, NULL, 16);
            src += 3;
        } else if (*src == '+') {
            dst[i++] = ' ';
            src++;
        } else {
            dst[i++] = *src++;
        }
    }
    dst[i] = '\0';
}
