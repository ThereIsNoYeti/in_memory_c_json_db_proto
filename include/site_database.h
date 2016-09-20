#ifndef REQUEST_CHANNEL
#define REQUEST_CHANNEL
    #include <czmq.h>
    #include <string.h>
    #include <sqlite3.h>
    #include <json/json.h>
    #include <malloc.h>

    #define SQL_ERRCHECK {if (sql_err_buf || rc) {printf("A SQL error has occured! \n\tError code: %d\n\tMessage: %s\n", rc, sql_err_buf); sqlite3_free(sql_err_buf);}}
    #define DEFAULT_STRING_LENGTH 64

    typedef struct { char *id, *domain, *settings;
                     sqlite3 *database_handle; } site_object;

    extern site_object * new_site();
    extern int free_site_object(site_object *site);

    extern int site_update_from_json(site_object *self, json_object *site_json);
    extern int site_update_from_string(site_object *self, const char *site_desc);
    int sql_results_to_json(void *out_ptr, int columns, char **column_values, char **column_names);
    char * strautocat(char **buffer, const char *str1);

    extern char * freeswitch_configuration_request(const char * request_section, const char *param); //request_string == dialplan|directory|configuration|phrases
    extern char * dialplan_request(const char * dest_string);

#endif