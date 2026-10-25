/* ============================================================
   ai.c — AI Chatbot Proxy Implementation
   Uses WinHTTP to call Groq API over HTTPS
   Demonstrates: Windows API usage, HTTPS client, JSON building
   ============================================================ */

#include "ai.h"
#include "http.h"

#ifdef _WIN32
  #include <winhttp.h>
  #pragma comment(lib,"winhttp.lib")

void api_chat(SOCKET fd, const char *body, MYSQL *db) {
    (void)db; /* not used for chat */

    /* Extract message and model from POST body */
    char msg[1024] = {0}, model[128] = {0};
    jget(body, "message", msg, sizeof(msg));
    jget(body, "model", model, sizeof(model));
    if (!model[0]) strcpy(model, "llama3-70b-8192");
    if (!msg[0]) {
        send_resp(fd, "400 Bad Request", "application/json",
            "{\"reply\":\"Empty message\"}");
        return;
    }

    /* Build system prompt */
    char sys_prompt[512];
    snprintf(sys_prompt, sizeof(sys_prompt),
        "You are PlantBot, an expert plant care assistant for the Smart Plant Manager app. "
        "You help users with plant care, watering schedules, soil types, plant diseases, "
        "and gardening tips. Keep answers concise and friendly. Use emojis occasionally.");

    /* Escape strings for JSON embedding */
    char esc_msg[2048] = {0}, esc_sys[1024] = {0}, esc_model[256] = {0};
    json_escape(msg, esc_msg, sizeof(esc_msg));
    json_escape(sys_prompt, esc_sys, sizeof(esc_sys));
    json_escape(model, esc_model, sizeof(esc_model));

    /* Build Groq API request JSON */
    char req_json[4096];
    snprintf(req_json, sizeof(req_json),
        "{\"model\":\"%s\","
        "\"messages\":["
          "{\"role\":\"system\",\"content\":\"%s\"},"
          "{\"role\":\"user\",\"content\":\"%s\"}"
        "],"
        "\"max_tokens\":512,"
        "\"temperature\":0.7}",
        esc_model, esc_sys, esc_msg);

    /* Build Authorization header */
    char auth_header[256];
    snprintf(auth_header, sizeof(auth_header), "Authorization: Bearer %s", GROQ_KEY);

    /* Open WinHTTP session */
    HINTERNET hSess = WinHttpOpen(L"PlantManager/2.0",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);

    if (!hSess) {
        send_resp(fd, "500 Error", "application/json",
            "{\"reply\":\"AI service unavailable. Check your internet connection.\"}");
        return;
    }

    HINTERNET hConn = WinHttpConnect(hSess, GROQ_HOST, INTERNET_DEFAULT_HTTPS_PORT, 0);
    HINTERNET hReq  = WinHttpOpenRequest(hConn, L"POST",
        L"/openai/v1/chat/completions",
        NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);

    /* Set headers */
    wchar_t wauth[512];
    mbstowcs(wauth, auth_header, 512);
    WinHttpAddRequestHeaders(hReq, wauth, -1, WINHTTP_ADDREQ_FLAG_ADD);
    WinHttpAddRequestHeaders(hReq, L"Content-Type: application/json", -1, WINHTTP_ADDREQ_FLAG_ADD);

    /* Send request */
    BOOL ok = WinHttpSendRequest(hReq, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
        (LPVOID)req_json, (DWORD)strlen(req_json), (DWORD)strlen(req_json), 0);

    if (!ok || !WinHttpReceiveResponse(hReq, NULL)) {
        WinHttpCloseHandle(hReq);
        WinHttpCloseHandle(hConn);
        WinHttpCloseHandle(hSess);
        send_resp(fd, "500 Error", "application/json",
            "{\"reply\":\"Could not reach Groq API. Check internet and API key.\"}");
        return;
    }

    /* Read response body */
    static char resp_buf[65536];
    resp_buf[0] = '\0';
    DWORD avail = 0, read_bytes = 0;
    int total = 0;
    while (WinHttpQueryDataAvailable(hReq, &avail) && avail > 0
           && total < (int)sizeof(resp_buf) - 1) {
        int to_read = avail < (DWORD)(sizeof(resp_buf) - total - 1)
                    ? avail : sizeof(resp_buf) - total - 1;
        WinHttpReadData(hReq, (LPVOID)(resp_buf + total), (DWORD)to_read, &read_bytes);
        total += read_bytes;
    }
    resp_buf[total] = '\0';

    WinHttpCloseHandle(hReq);
    WinHttpCloseHandle(hConn);
    WinHttpCloseHandle(hSess);

    /* Extract "content" field from nested Groq JSON response */
    const char *content_key = "\"content\":\"";
    char *cstart = strstr(resp_buf, content_key);
    if (!cstart) {
        char errmsg[512] = {0};
        jget(resp_buf, "message", errmsg, sizeof(errmsg));
        if (errmsg[0]) {
            char out[600];
            snprintf(out, sizeof(out), "{\"reply\":\"Groq error: %s\"}", errmsg);
            send_resp(fd, "200 OK", "application/json", out);
        } else {
            send_resp(fd, "200 OK", "application/json",
                "{\"reply\":\"Sorry, I could not get a response. Try again!\"}");
        }
        return;
    }
    cstart += strlen(content_key);

    /* Extract until closing quote (handle escaped quotes) */
    static char reply[4096];
    int ri = 0;
    while (*cstart && ri < (int)sizeof(reply) - 1) {
        if (*cstart == '\\' && *(cstart + 1) == '"') {
            reply[ri++] = '"'; cstart += 2;
        } else if (*cstart == '\\' && *(cstart + 1) == 'n') {
            reply[ri++] = '\n'; cstart += 2;
        } else if (*cstart == '"') {
            break;
        } else {
            reply[ri++] = *cstart++;
        }
    }
    reply[ri] = '\0';

    /* Send response as JSON */
    static char out[8192];
    char esc_reply[4096] = {0};
    json_escape(reply, esc_reply, sizeof(esc_reply));
    snprintf(out, sizeof(out), "{\"reply\":\"%s\"}", esc_reply);
    send_resp(fd, "200 OK", "application/json", out);
}

#else
/* Non-Windows fallback */
void api_chat(SOCKET fd, const char *body, MYSQL *db) {
    (void)body; (void)db;
    send_resp(fd, "200 OK", "application/json",
        "{\"reply\":\"AI chat requires Windows with WinHTTP.\"}");
}
#endif
