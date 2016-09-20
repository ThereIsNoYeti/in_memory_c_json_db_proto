#define LONG_STRING(...) #__VA_ARGS__
#define PRESS_TO_CONT printf("Press any key to proceed..."); getchar();

#include <time.h>
#include <stdio.h>
#include <assert.h>

#include "../include/sandbox.h"
#include "../include/site_database.h"


int main() {

    printf("\n\n\n");

    //-- Json parsing, orfinarily I'd just parse from a string ---------------------------------------------------------
    json_object *site_json;
        site_json = json_object_from_file("/home/juston/workspace/teliax/bonsai/data/sites/sip.teliax.tv.json");
        // site_json = json_tokener_parse(string);                                                                      //char * string = LONG_STRING({stuff}); //Just a way to inject a json string without escaping it

    parse_site_desc(site_json);
    json_object_put(site_json); //frees

    printf("--------------------------------------------------\n");

    //-- String util stuff Im going to need for query assembly ---------------------------------------------------------
    const char *str1, *str2; char *buffer, *buffer_null;
        str1 = "Revenge is a dish best served";
        str2 = "...constantly";
        buffer = (char *)calloc(sizeof(char)*DEFAULT_STRING_LENGTH,sizeof(char));
        buffer_null = 0;

    buffer = sandbox_strautocat(&buffer, str1);
    buffer = sandbox_strautocat(&buffer, str2);

    buffer_null = sandbox_strautocat(&buffer_null, str1);
    buffer_null = sandbox_strautocat(&buffer_null, str2);

    printf("Here is the output => %s\n", buffer);
    printf("Here is the output => %s\n", buffer_null);

    free(buffer);
    free(buffer_null);
    printf("--------------------------------------------------\n");

    //-- SQLite3 stuff, lets open a bunch of in-memory handles ---------------------------------------------------------
    int rc, times;
    clock_t start, end;
    const char *query1, *query2, *query3; char *sql_err_buf, *sql_return_buff;
        query1 = "DROP TABLE IF EXISTS features; CREATE TABLE features(id  INTEGER PRIMARY KEY NOT NULL, core_id INTEGER NOT NULL, core_type TEXT NOT NULL, trigger_value TEXT, 'values' TEXT);";
        query2 = "INSERT INTO features (id, core_id, core_type, trigger_value, 'values') VALUES (NULL, '-1', 'test_feature', 'test', '{\"number\":\"17207081501\",\"destination\":\"device:57\"}');";
        query3 = "SELECT * from features where core_type='test_feature' and core_id='-1'";
        sql_err_buf = 0;
        sql_return_buff = 0;
   json_object *sql_json_buff;
        sql_json_buff = json_object_new_object();
    sqlite3 *database_handle;
        rc = sqlite3_open(":memory:", &database_handle); assert(rc== SQLITE_OK);    //It might not be necissary to cache=shared since I imagine only 1 thread will be using these at first

    printf("Running query => %s\n", query1);
        start = clock();
        rc = sqlite3_exec(database_handle, query1, NULL, NULL, &sql_err_buf);
        ERRCHECK; PRNTVALS;
        end = clock(); printf("\tElapsed time: %.2f seconds\n", (double)(end - start) / CLOCKS_PER_SEC);

    printf("Running %d copies of query => %s \n", 100, query2);
        start = clock();
        for(times = 100; times > 0; times--)
        {
            rc = sqlite3_exec(database_handle, query2, NULL, NULL, &sql_err_buf);
            ERRCHECK; PRNTVALS;
        }
        end = clock(); printf("\tElapsed time: %.2f seconds\n", (double)(end - start) / CLOCKS_PER_SEC);
    printf("Running query => %s\n", query3);
        start = clock();
         rc = sqlite3_exec(database_handle, query3, json_sql_results, (void*)&sql_json_buff, &sql_err_buf);
         //rc = sqlite3_exec(database_handle, query3, print_sql_results, (void*)&sql_return_buff, &sql_err_buf);
        ERRCHECK; PRNTJSONVALS;
        end = clock(); printf("\tElapsed time: %.2f seconds\n", (double)(end - start) / CLOCKS_PER_SEC);
    sqlite3_close(database_handle);
    printf("--------------------------------------------------\n");


    //-- Zhash prototypes ----------------------------------------------------------------------------------------------
    printf("Parsing the site\n");
        start = clock();
        site_object *site_0 = new_site();
        json_object *site_0_json = json_object_from_file("/home/juston/workspace/teliax/bonsai/data/sites/sip.teliax.tv.json");
            //site_update_from_json(site_0, site_0_json);
            site_update_from_string(site_0, json_object_to_json_string_ext(site_0_json, JSON_C_TO_STRING_PRETTY));
        json_object_put(site_0_json);
        end = clock(); printf("\tElapsed time: %.2f seconds\n", (double)(end - start) / CLOCKS_PER_SEC);
    printf("Releasing the site\n");
        start = clock();
        free_site_object(site_0);
        end = clock(); printf("\tElapsed time: %.2f seconds\n", (double)(end - start) / CLOCKS_PER_SEC);

    //PRESS_TO_CONT
    return 0;
}