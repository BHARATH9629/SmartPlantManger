#ifndef ENV_H
#define ENV_H

/* ============================================================
   env.h — Environment Variable and .env File Loader
   Safely loads database credentials and API keys from a local
   .env file or system environment variables without exposing
   them in source code.
   ============================================================ */

/* Load key=value pairs from a .env file into the process environment */
void load_env(const char *filename);

/* General environment getters with fallbacks */
const char *get_env_str(const char *key, const char *default_val);
int         get_env_int(const char *key, int default_val);

/* Project-specific configuration getters */
const char *get_db_host(void);
const char *get_db_user(void);
const char *get_db_pass(void);
const char *get_db_name(void);
const char *get_groq_key(void);
int         get_server_port(void);

#endif /* ENV_H */
