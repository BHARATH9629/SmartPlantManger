/* ============================================================
   env.c — Environment Variable and .env File Loader Implementation
   ============================================================ */

#include "env.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#ifdef _WIN32
  #include <windows.h>
#endif

/* Trim whitespace from both ends of a string in-place */
static char *trim_whitespace(char *str) {
    char *end;
    while (isspace((unsigned char)*str)) str++;
    if (*str == 0) return str;
    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    end[1] = '\0';
    return str;
}

void load_env(const char *filename) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        return; /* .env file is optional; falls back to system env or defaults */
    }

    char line[1024];
    while (fgets(line, sizeof(line), fp)) {
        char *trimmed = trim_whitespace(line);
        if (trimmed[0] == '\0' || trimmed[0] == '#') {
            continue; /* Skip empty lines and comments */
        }

        char *eq = strchr(trimmed, '=');
        if (!eq) continue;

        *eq = '\0';
        char *key = trim_whitespace(trimmed);
        char *val = trim_whitespace(eq + 1);

        if (key[0] == '\0') continue;

        /* Strip surrounding single or double quotes */
        size_t len = strlen(val);
        if (len >= 2 && ((val[0] == '"' && val[len - 1] == '"') ||
                         (val[0] == '\'' && val[len - 1] == '\''))) {
            val[len - 1] = '\0';
            val++;
        }

        /* Update Win32 and C runtime environments */
#ifdef _WIN32
        SetEnvironmentVariableA(key, val);
        char *putenv_str = (char *)malloc(strlen(key) + strlen(val) + 2);
        if (putenv_str) {
            sprintf(putenv_str, "%s=%s", key, val);
            _putenv(putenv_str);
        }
#else
        setenv(key, val, 1);
#endif
    }
    fclose(fp);
}

const char *get_env_str(const char *key, const char *default_val) {
    const char *v = getenv(key);
    if (v && v[0] != '\0') return v;
    return default_val;
}

int get_env_int(const char *key, int default_val) {
    const char *v = getenv(key);
    if (v && v[0] != '\0') {
        int n = atoi(v);
        if (n > 0) return n;
    }
    return default_val;
}

const char *get_db_host(void) {
    return get_env_str("DB_HOST", "localhost");
}

const char *get_db_user(void) {
    return get_env_str("DB_USER", "root");
}

const char *get_db_pass(void) {
    return get_env_str("DB_PASS", "");
}

const char *get_db_name(void) {
    return get_env_str("DB_NAME", "plant");
}

const char *get_groq_key(void) {
    return get_env_str("GROQ_API_KEY", "");
}

int get_server_port(void) {
    return get_env_int("PORT", 9000);
}
