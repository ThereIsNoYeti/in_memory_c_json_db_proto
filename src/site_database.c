#include "../include/site_database.h"


extern site_object * new_site()
{
    site_object *self;
        self = (site_object *)malloc(sizeof(site_object)); assert(self);

    return self;
}

extern int site_update_from_json(site_object *self, json_object *site_json)
{
    assert(self); assert(site_json);

    int rc;
    json_object *site, *domain, *settings, *features, *feature;

    void *sql_callback_function, *sql_callback_param;
    const char *sql_create_query, *sql_feature_fmt, *sql_connect_fmt; char *sql_err_buf, *sql_connect_buff, *sql_query_buff;
        sql_connect_fmt  = ":memory:";
        sql_feature_fmt  = "INSERT INTO features (id, core_id, core_type, trigger_value, 'values') VALUES (NULL, '%s', '%s', '%s', '%s');";
        sql_create_query = "DROP TABLE IF EXISTS features; CREATE TABLE features( id INTEGER PRIMARY KEY NOT NULL, core_id INTEGER NOT NULL, core_type TEXT NOT NULL, trigger_value TEXT, 'values' TEXT);";
        sql_connect_buff = (char *)malloc(sizeof(char)*75);  //should allow for 10^12 unique site databases without a buffer cut-off issue
        sql_err_buf = 0; sql_query_buff= 0; sql_callback_function = 0; sql_callback_param = 0;

    //-- Parse and storing JSON values ----------
    {
        printf("Parsing site JSON...\n");
        //TODO: site_json = json_tokener_parse_verbose(json_string); assert(site_json);
        //site_json = json_object_from_file("/home/juston/workspace/teliax/bonsai/data/sites/sip.teliax.tv.json");

        assert(json_object_object_get_ex(site_json, "site", &site));
        assert(json_object_object_get_ex(site_json, "domain", &domain));
        assert(json_object_object_get_ex(site_json, "settings", &settings));
        assert(json_object_object_get_ex(site_json, "features", &features));

        //The strings from json_object_get_string are memory managed by the chief json_object, they get dealloced when I free it
        self->id = (char *)malloc(sizeof(char)*strlen(json_object_get_string(site))+1);
            strcpy(self->id , json_object_get_string(site));
        self->domain = (char *)malloc(sizeof(char)*strlen(json_object_get_string(domain))+1);
            strcpy(self->domain, json_object_get_string(domain));
        self->settings = (char *)malloc(sizeof(char)*strlen(json_object_get_string(settings))+1);
            strcpy(self->settings, json_object_get_string(settings));

        printf("Site created: \n\tsite id => '%s'\n\tsite domain => '%s'\n\tsite settings => '%s'\n",
                self->id,
                self->domain,
                self->settings);

        // -- Block frees -----
            //None
    }

    //-- Database creation and insert ----------
    {
        snprintf (sql_connect_buff, 74, sql_connect_fmt, self->id); sql_connect_buff[74] = '\0';

        printf("Connecting site database: %s\n", sql_connect_buff);
        rc = sqlite3_open(":memory:", &self->database_handle);

        if(self->database_handle)
        {
            printf("Executing queries...\n");

            printf("\tCreating features table... \n\t\t%s\n", sql_create_query);
//            rc = sqlite3_exec(self->database_handle, sql_create_query, sql_callback_function, (void*)&sql_callback_param, &sql_err_buf); SQL_ERRCHECK;
            rc = sqlite3_exec(self->database_handle, sql_create_query, NULL, (void*)&sql_callback_param, &sql_err_buf); SQL_ERRCHECK;

            printf("\tBuilding features... \n");
            {
                int i, array_length, statement_format_len, feature_query_len;
                    statement_format_len = strlen(sql_feature_fmt) - 8; //Speeds things up not to calculate this each time, minus 8 for the '%s's
                    array_length = json_object_array_length(features);
                char *feature_query_str;


                for (i=0; i< array_length; i++){

                    feature = json_object_array_get_idx(features, i);

                    json_object *core_id_json_obj, *core_type_json_obj, *trigger_value_json_obj, *values_json_obj;
                        core_id_json_obj       = json_object_object_get(feature, "core_id");
                        core_type_json_obj     = json_object_object_get(feature, "core_type");
                        trigger_value_json_obj = json_object_object_get(feature, "trigger_value");
                        values_json_obj        = json_object_object_get(feature, "values");

                    const char *core_id_s, *core_type_s, *trigger_value_s, *values_s;
                        core_id_s       = json_object_get_string(core_id_json_obj);
                        core_type_s     = json_object_get_string(core_type_json_obj);
                        trigger_value_s = json_object_get_string(trigger_value_json_obj);
                        values_s        = json_object_get_string(values_json_obj);

                        core_id_s       = (core_id_s ? core_id_s : "");
                        core_type_s     = (core_type_s ? core_type_s : "");
                        trigger_value_s = (trigger_value_s ? trigger_value_s : "");
                        values_s        = (values_s ? values_s : "");

                    feature_query_len = statement_format_len + strlen(core_id_s) + strlen(core_type_s) + strlen(trigger_value_s) + strlen(values_s) + 1;
                    feature_query_str = (char *)malloc(sizeof(char)*feature_query_len);

                    snprintf( feature_query_str, feature_query_len, sql_feature_fmt, core_id_s, core_type_s, trigger_value_s, values_s);

                    strautocat(&sql_query_buff,feature_query_str);
                    free(feature_query_str);
                }
                printf("\tRunning feature insert/update query... \n\t\t%s\n", sql_query_buff);
                rc = sqlite3_exec(self->database_handle, sql_query_buff, NULL, (void*)&sql_callback_param, &sql_err_buf); SQL_ERRCHECK;

                // -- Block frees -----
                free(sql_query_buff);
            }
        }
        else
        {
            printf("Connection failed with error code: %d", rc);
        }
    }
    // -- Block frees -----
    free(sql_connect_buff);
    return 0;
}

extern int site_update_from_string(site_object *self, const char *site_desc)
{
  int rc;
  json_object *site_json;
    site_json = json_tokener_parse(site_desc); assert(site_json);

  rc = site_update_from_json(self, site_json);

  json_object_put(site_json);
  return rc;
}

extern json_object * sql_query(site_object *self, const char *query_string)
{
    assert(self); assert(query_string);

    int rc;
    char *sql_err_buf;
    json_object *sql_json_buff;
        sql_json_buff = json_object_new_object();

    rc = sqlite3_exec(self->database_handle, query_string, sql_results_to_json, (void*)&sql_json_buff, &sql_err_buf);
    SQL_ERRCHECK;

    free(sql_err_buf);
    return sql_json_buff;
}

//char * freeswitch_configuration_request(const char * request_section, const char *param){} //request_string == dialplan|directory|configuration|phrases

//char * dialplan_request(const char * dest_string)
//{
///* -- Lua dialplan.lua replacement, expect 'destination' as the only parameter */
///*1.) get site feature desc
//    -determine what is being asked for
//            -special case ('limit_exceeded', '*9(\d{4})', '911|933'
//            -class:id
//            -trigger
//
//    -return 'catch_all' and break if not found
//  2.) get the corresponding feature execution code
//
//*/
//    const char *sql_feature_fmt, *feature_type, *feature_id; //regexp matches
//        sql_feature_fmt = ""
//
//        return "";
//}

extern int free_site_object(site_object *site)
{
    free(site->id);
    free(site->domain);
    free(site->settings);
    sqlite3_close(site->database_handle);

    free(site);
    return 0;
}

int sql_results_to_json(void *out_ptr, int columns, char **column_values, char **column_names){
    assert(out_ptr);

    int i;
    json_object **doc_obj, *results_arr, *result_obj, *key_s, *val_s;
        doc_obj = (json_object**) out_ptr;
            if(doc_obj == NULL){*doc_obj = json_object_new_object();}
        results_arr = json_object_object_get(*doc_obj, "results");
            if(results_arr==NULL){results_arr = json_object_new_array(); json_object_object_add(*doc_obj, "results", results_arr);}

    result_obj  = json_object_new_object();
    for(i=0; i<columns; i++)
    {
        val_s = json_object_new_string((column_values[i] ? column_values[i] : "NULL"));
        json_object_object_add(result_obj,column_names[i], val_s);
    }
    json_object_array_add(results_arr, result_obj);
    return 0;
}

char * strautocat(char **buffer, const char *str1)  //char **buffer, const char *format, vargs? Ive done this before
{
    assert(str1 != NULL); assert(buffer != NULL);
    if(*buffer == NULL){*buffer = (char *)calloc(sizeof(char)*DEFAULT_STRING_LENGTH,sizeof(char));}

    int reallocate_required;
       reallocate_required = 0;
    size_t allocated_size, required_size;
        allocated_size = malloc_usable_size(*buffer);
        required_size  = strlen(str1)+strlen(*buffer)+1;

    while(required_size > allocated_size)
    {
        reallocate_required = 1;
        allocated_size = allocated_size*2;
    }

    if(reallocate_required)
    {
        printf("Growing buffer to %d\n", allocated_size);
        *buffer = (char *)realloc(*buffer, sizeof(char)*allocated_size);
     }
    strcat(*buffer, str1);
    return *buffer;
}