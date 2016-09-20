//-- JSON parsing/Query gen --------------------------------------------------------------------------------------------
#include <json/json.h>
void parse_site_desc(json_object *site_json)
{
    //printf("%s\n", json_object_to_json_string_ext(jobj, JSON_C_TO_STRING_PRETTY));
    printf("Inserting site entry into the core (not requred with a zhash)\n");

    json_object *site     = json_object_object_get(site_json, "site");
    json_object *domain   = json_object_object_get(site_json, "domain");
    json_object *settings = json_object_object_get(site_json, "settings");
    json_object *features = json_object_object_get(site_json, "features");

    //I should assert on  enum json_type type = json_object_get_type(jobj); being {json_type_boolean, json_type_double json_type_int, json_type_string} and possibly casing but I dont

    printf(
            "INSERT OR REPLACE INTO sites (id, domain, database_file, encrypted_settings) VALUES ('%s', '%s', 'site_%s.db', '%s');\n\0",
            json_object_get_string(site),
            json_object_get_string(domain),
            json_object_get_string(site),
            json_object_get_string(settings)
           );

    printf("\n");
    printf("Inserting feature entries into the site database\n"); //I guess this might not be required, but it sure is handy to reference this stuff via sql statements

    int i, query_length, array_length;
    json_object * feature;

    array_length = json_object_array_length(features);

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

        printf(
                "INSERT INTO features (id, core_id, core_type, trigger_value, encrypted_values) VALUES (NULL, '%s', '%s', '%s', '%s');\n\0",
                core_id_s,
                core_type_s,
                trigger_value_s,
                values_s
              );
    }

    printf("\n");
    printf("Freeing memory...\n");
    //The json objects are just pointers to positions in the cheifton json object, no need to free those
    //The ints don't need to be freed since they are scoped

    printf("\n");
    printf("Site paring complete!\n");

    return;
}

//-- String concat util, slow as shit ----------------------------------------------------------------------------------
#define DEFAULT_STRING_LENGTH 1

#include <string.h>
#include <malloc.h>

char * sandbox_strautocat(char **buffer, const char *str1)  //char **buffer, const char *format, vargs? Ive done this before
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

int str_alloc_copy(char **dest, const char *src)
{
    assert(src != NULL); assert(dest != NULL);

    *dest = (char *)malloc(sizeof(char)*strlen(src)+1);
//
//    size_t allocated_size, required_size;
//        allocated_size = malloc_usable_size(*dest);
//        required_size  = strlen(src)+1;

//    if (required_size > allocated_size){
//        *dest = (char *)realloc(*dest, sizeof(char)*required_size);
//    }

  strncpy(*dest, src, strlen(src)+1);
  return 0;
}

//-- SQLite3 result callbacks ------------------------------------------------------------------------------------------
#define PRNTVALS     {if (sql_return_buff!=NULL){free(sql_return_buff);}}
#define PRNTJSONVALS {if (sql_json_buff!=NULL)  {json_object_to_json_string_ext(sql_json_buff, JSON_C_TO_STRING_PRETTY);json_object_put(sql_json_buff);}}

#define ERRCHECK {if (sql_err_buf!=NULL)     {printf("%s\n",sql_err_buf); sqlite3_free(sql_err_buf);}}
//#define PRNTVALS {if (sql_return_buff!=NULL) {printf("%s\n",sql_return_buff); free(sql_return_buff);}}
//#define PRNTJSONVALS {if (sql_json_buff!=NULL) {printf("%s\n",json_object_to_json_string_ext(sql_json_buff, JSON_C_TO_STRING_PRETTY)); json_object_put(sql_json_buff);}}

#include <malloc.h>
#include <sqlite3.h>
#include <json/json.h>

static int print_sql_results(void *data, int columns, char **column_values, char **column_names){
    int i;
    char **buffer;
        buffer = (char **) data; if(*buffer == NULL){ *buffer = (char *)calloc(sizeof(char)*DEFAULT_STRING_LENGTH,sizeof(char));}
       sandbox_strautocat(buffer, "Result\n");
    for(i=0; i<columns; i++)
    {
        sandbox_strautocat(buffer, "\t");
        sandbox_strautocat(buffer, column_names[i]);
        sandbox_strautocat(buffer, " => ");
        sandbox_strautocat(buffer, (column_values[i] ? column_values[i] : "NULL"));
        sandbox_strautocat(buffer, "\n");
    }
    return 0;
}

static int json_sql_results(void *object_pp, int columns, char **column_values, char **column_names){
    assert(object_pp != NULL);

    int i;
    json_object **doc_obj, *results_arr, *result_obj, *key_s, *val_s;
        doc_obj = (json_object**) object_pp;
            if(doc_obj == NULL){*doc_obj = json_object_new_object();}
        results_arr = json_object_object_get(*doc_obj, "results");
            if(results_arr==NULL){results_arr = json_object_new_array(); json_object_object_add(*doc_obj, "results", results_arr);}

//Creat result object and add each key/value to it
    result_obj  = json_object_new_object();
    for(i=0; i<columns; i++)
    {
        val_s = json_object_new_string((column_values[i] ? column_values[i] : "NULL"));
        json_object_object_add(result_obj,column_names[i], val_s);
    }
    json_object_array_add(results_arr, result_obj);
    return 0;
}