#include "../includes/db.h"
#include "../includes/parson.h"

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <linux/limits.h>

char*
table_exist(
    const char *table_name
) {
    char *path = (char*)malloc(PATH_MAX);
    if (getcwd(path, PATH_MAX) != NULL) {
        sprintf(path, "%s/db/%s.json", path, table_name);
        FILE *file;
        if (file = fopen(path, "r")) {
            fclose(file);
            return path;
        }
    }
    return NULL;
}

char*
get_data(
    char *table_path,
    int index,
    char *search
) {
    JSON_Value *table = json_parse_file(table_path);

    if (index != -1) {
        JSON_Array *rows = json_value_get_array(table);
        if (index > ((int)json_array_get_count(rows) - 1)) {
            char *error = (char*)malloc(36);
            sprintf(error, "\"Index `%d` out of table.\"", index);
            return error;
        }
        return json_serialize_to_string(json_array_get_value(rows, index));
    }
    else if (search != NULL) {
        JSON_Array *rows = json_value_get_array(table);
        JSON_Value *search_value = json_parse_string(search);
        if (json_validate(json_parse_string("[{\"key\":\"\",\"value\":null}]"), search_value) != JSONSuccess) {
            char *error = (char*)malloc(strlen(search) + 38);
            sprintf(error, "\"Search `%s` not valid for this table.\"", search);
            return error;
        }
        JSON_Array *search_array = json_value_get_array(search_value);
        size_t search_count = json_array_get_count(search_array);
        size_t count = json_array_get_count(rows);
        for (int i = 0; i < (int)count; i++) {
            int equals = 0;
            JSON_Object *row_object = json_array_get_object(rows, i);
            for (size_t n = 0; n < search_count; n++) {
                JSON_Object *search_object = json_array_get_object(search_array, n);
                equals = json_value_equals(
                    json_object_get_value(row_object, json_object_get_string(search_object, "key")), 
                    json_object_get_value(search_object, "value")
                );
            }
            if (equals == 0) {
                if (json_array_remove(rows, i) == JSONSuccess) {
                    count--;
                    i--;
                }
            }
        }
        return json_serialize_to_string(table);
    }
    else
        return json_serialize_to_string(table);
}

char*
insert_data(
    char *table_path,
    const char *request
) {
    JSON_Value *table = json_parse_file(table_path);
    JSON_Array *rows = json_value_get_array(table);

    JSON_Value *row = json_parse_string(request);

    if (json_validate(json_array_get_value(rows, 0), row) != JSONSuccess) {
        json_value_free(table);
        char *response = (char*)malloc(strlen(request) + 46);
        sprintf(response, "\"Data to insert `%s` not valid for this table.\"", request);
        return response;
    }
    
    if (json_array_append_value(rows, row) == JSONSuccess) {
        if (json_serialize_to_file_pretty(table, table_path) == JSONSuccess) {
            json_value_free(table);
            printf("Row inserted.\n");
            char *response = (char*)malloc(strlen(request) + 14);
            sprintf(response, "\"Row `%s` inserted.\"", request);
            return response;
        }
        json_value_free(table);
    }
    
    return NULL;
}

char*
edit_data(
    char *table_path,
    int index,
    char *search,
    char *data
) {
    if (data == NULL)
        return NULL;
    
    if (index != -1) {
        JSON_Value *table = json_parse_file(table_path);
        JSON_Array *rows = json_value_get_array(table);
        if (index > ((int)json_array_get_count(rows) - 1)) {
            json_value_free(table);
            char *error = (char*)malloc(36);
            sprintf(error, "\"Index `%d` out of table.\"", index);
            return error;
        }

        JSON_Value *data_value = json_parse_string(data);
        if (json_validate(json_parse_string("[{\"key\":\"\",\"value\":null}]"), data_value) != JSONSuccess) {
            json_value_free(table);
            json_value_free(data_value);
            char *error = (char*)malloc(strlen(data) + 44);
            sprintf(error, "\"Data to edit `%s` not valid for this table.\"", data);
            return error;
        }
        
        JSON_Object *row = json_array_get_object(rows, index);
        JSON_Array *data_array = json_value_get_array(data_value);
        size_t data_count = json_array_get_count(data_array);
        for (size_t i = 0; i < data_count; i++) {
            JSON_Object *data_object = json_array_get_object(data_array, i);
            if (json_object_has_value_of_type(data_object, "value", JSONNumber) == 1)
                json_object_set_number(row, json_object_get_string(data_object, "key"), json_object_get_number(data_object, "value"));
            
            if (json_object_has_value_of_type(data_object, "value", JSONString) == 1)
                json_object_set_string(row, json_object_get_string(data_object, "key"), json_object_get_string(data_object, "value"));
        }

        if (json_serialize_to_file_pretty(table, table_path) == JSONSuccess) {
            json_value_free(table);
            json_value_free(data_value);
            char *response = (char*)malloc(37);
            sprintf(response, "\"Row at index `%d` edited.\"", index);
            return response;
        }

        json_value_free(table);
        json_value_free(data_value);

        return NULL;
    }
    else if (search != NULL) {
        JSON_Value *table = json_parse_file(table_path);
        JSON_Array *rows = json_value_get_array(table);

        JSON_Value *search_value = json_parse_string(search);
        if (json_validate(json_parse_string("[{\"key\":\"\",\"value\":null}]"), search_value) != JSONSuccess) {
            json_value_free(table);
            json_value_free(search_value);
            char *error = (char*)malloc(strlen(search) + 38);
            sprintf(error, "\"Search `%s` not valid for this table.\"", search);
            return error;
        }

        JSON_Value *data_value = json_parse_string(data);
        if (json_validate(json_parse_string("[{\"key\":\"\",\"value\":null}]"), data_value) != JSONSuccess) {
            json_value_free(table);
            json_value_free(data_value);
            json_value_free(search_value);
            char *response = (char*)malloc(strlen(data) + 44);
            sprintf(response, "\"Data to edit `%s` not valid for this table.\"", data);
            return response;
        }

        JSON_Array *search_array = json_value_get_array(search_value);
        size_t search_count = json_array_get_count(search_array);
        size_t count = json_array_get_count(rows);
        for (int i = 0; i < (int)count; i++) {
            int equals = 0;
            JSON_Object *row_object = json_array_get_object(rows, i);
            for (size_t n = 0; n < search_count; n++) {
                JSON_Object *search_object = json_array_get_object(search_array, n);
                equals = json_value_equals(
                    json_object_get_value(row_object, json_object_get_string(search_object, "key")), 
                    json_object_get_value(search_object, "value")
                );
            }
            if (equals == 1) {
                JSON_Array *data_array = json_value_get_array(data_value);
                size_t data_count = json_array_get_count(data_array);
                for (size_t i = 0; i < data_count; i++) {
                    JSON_Object *data_object = json_array_get_object(data_array, i);
                    if (json_object_has_value_of_type(data_object, "value", JSONNumber) == 1)
                        json_object_set_number(row_object, json_object_get_string(data_object, "key"), json_object_get_number(data_object, "value"));
                    
                    if (json_object_has_value_of_type(data_object, "value", JSONString) == 1)
                        json_object_set_string(row_object, json_object_get_string(data_object, "key"), json_object_get_string(data_object, "value"));
                }
            }
        }
        json_value_free(search_value);
        json_value_free(data_value);
        json_value_free(table);

        return NULL;
    }
    else
        return NULL;
}

char*
delete_data(
    char *table_path,
    int index,
    char *search
) {
    if (index < 0 && search == NULL) {
        JSON_Value *table = json_value_init_array();
        if (json_serialize_to_file_pretty(table, table_path) == JSONSuccess) {
            json_value_free(table);
            return "\"Table truncated.\"";
        }
        json_value_free(table); 
    } else if (index != -1) {
        JSON_Value *table = json_parse_file(table_path);
        JSON_Array *rows = json_value_get_array(table);
        if (index > ((int)json_array_get_count(rows) - 1)) {
            json_value_free(table);
            char *response = (char*)malloc(36);
            sprintf(response, "\"Index `%d` out of table.\"", index);
            return response;
        }
        if (json_array_remove(rows, index) == JSONSuccess) {
            if (json_serialize_to_file_pretty(table, table_path) == JSONSuccess) {
                json_value_free(table);
                char *response = (char*)malloc(38);
                sprintf(response, "\"Row at index `%d` deleted.\"", index);
                return response;
            }
            json_value_free(table);
        }
        json_value_free(table);
    } else if (search != NULL) {
        JSON_Value *search_value = json_parse_string(search);
        if (json_validate(json_parse_string("[{\"key\":\"\",\"value\":null}]"), search_value) != JSONSuccess) {
            json_value_free(search_value);
            char *error = (char*)malloc(strlen(search) + 38);
            sprintf(error, "\"Search `%s` not valid for this table.\"", search);
            return error;
        }
        JSON_Value *table = json_parse_file(table_path);
        JSON_Array *rows = json_value_get_array(table);

        JSON_Array *search_array = json_value_get_array(search_value);
        size_t search_count = json_array_get_count(search_array);

        size_t count = json_array_get_count(rows);
        for (int i = 0; i < (int)count; i++) {
            int equals = 0;
            JSON_Object *row_object = json_array_get_object(rows, i);
            for (size_t n = 0; n < search_count; n++) {
                JSON_Object *search_object = json_array_get_object(search_array, n);
                equals = json_value_equals(
                    json_object_get_value(row_object, json_object_get_string(search_object, "key")), 
                    json_object_get_value(search_object, "value")
                );
            }
            if (equals == 1) {
                if (json_array_remove(rows, i) == JSONSuccess) {
                    count--;
                    i--;
                }
            }
        }
        json_value_free(search_value);

        if (json_serialize_to_file_pretty(table, table_path) == JSONSuccess) {
            json_value_free(table);
            char *response = (char*)malloc(strlen(search) + 30);
            sprintf(response, "\"Rows for search `%s` deleted.\"", search);
            return response;
        }
        json_value_free(table);
    }

    return NULL;
}