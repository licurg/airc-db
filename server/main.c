#include "../includes/server.h"
#include "../includes/http_parser.h"
#include "../includes/parson.h"
#include "../includes/db.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>

int
send_error_response(
    int *socket_fd,
    char *message
) {
    unsigned int message_length = strlen(message); 
    char *response = malloc(message_length + 96);
    sprintf(response, "HTTP/1.1 500 Internal Server Error\nContent-Type: application/json\nContent-Length: %d\n\n%s", message_length, message);
    return write(*socket_fd, response, strlen(response));
}

int
send_success_response(
    int *socket_fd,
    char *message
) {
    unsigned int message_length = strlen(message);
    char *response = malloc(message_length + 77);
    sprintf(response, "HTTP/1.1 200 OK\nContent-Type: application/json\nContent-Length: %d\n\n%s", message_length, message);
    return write(*socket_fd, response, strlen(response));
}

int
send_custom_response(
    int *socket_fd,
    char *response
) {
    return write(*socket_fd, response, strlen(response));
}

int 
on_body(
    http_parser* parser, 
    const char* at, 
    size_t length
) {
    (void)parser;
    if ((int)length > 0) {
        parser->data = (char*)malloc(length);
        memcpy(parser->data, at, length);
    }
    return 0;
}

void
*handler(
    void *socket
) {
    int socket_fd = *(int*)socket;
    struct http_parser_settings *http_settings = (struct http_parser_settings*)malloc(sizeof(struct http_parser_settings));
    struct http_parser *http_parser = (struct http_parser*)malloc(sizeof(struct http_parser));
    http_parser_init(http_parser, HTTP_REQUEST);

    size_t length = 80 * 1024, http_request_length, http_parser_length;
    char *buffer = (char*)malloc(length);
    http_request_length = recv(socket_fd, buffer, length, 0);
    if (http_request_length < 0) {
        printf("\033[0;31mEmpty request.\033[0m\n");
        if (send_error_response(&socket_fd, "\"Empty request.\"") < 0)
            printf("\033[0;31mError response failed.\033[0m\n");

        free(http_parser);
        free(http_settings);
        free(socket);
        return 0;
    }

    http_settings->on_body = on_body;
    http_parser_length = http_parser_execute(http_parser, http_settings, buffer, http_request_length);
    if (http_parser_length != http_request_length) {
        printf("\033[0;31mHTTP parse failed.\033[0m\n");
        if (send_error_response(&socket_fd, "\"HTTP parse failed.\"") < 0)
            printf("\033[0;31mError response failed.\033[0m\n");

        free(http_parser);
        free(http_settings);
        free(socket);
        return 0;
    }

    if (http_parser->data == NULL || strlen(http_parser->data) == 0) {
        printf("\033[0;31mEmpty request body.\033[0m\n");
        if (send_error_response(&socket_fd, "\"Empty request body.\"") < 0)
            printf("\033[0;31mError response failed.\033[0m\n");

        free(http_parser);
        free(http_settings);
        free(socket);
        return 0;
    }

    JSON_Value *json = json_parse_string(http_parser->data);
    JSON_Object *json_object = json_value_get_object(json);
    if (json_object_has_value(json_object, "target") == 0) {
        printf("\033[0;31mNo target.\033[0m\n");
        if (send_error_response(&socket_fd, "\"Please fill `target` field.\"") < 0)
            printf("\033[0;31mError response failed.\033[0m\n");
        
        json_object_clear(json_object);
        json_value_free(json);
        free(http_parser);
        free(http_settings);
        free(socket);
        return 0;
    }

    const char *target = json_object_get_string(json_object, "target");
    if (strcmp(target, "db") == 0) {
        if (json_object_has_value_of_type(json_object, "table", JSONString) == 0) {
            printf("\033[0;31mNo table.\033[0m\n");
            if (send_error_response(&socket_fd, "\"Please fill `table` field.\"") < 0)
                printf("\033[0;31mError response failed.\033[0m\n");
            
            json_object_clear(json_object);
            json_value_free(json);
            free(http_parser);
            free(http_settings);
            free(socket);
            return 0;
        }
        char *table_path;
        if ((table_path = table_exist(json_object_get_string(json_object, "table"))) == NULL) {
            printf("\033[0;31mTable not exist.\033[0m\n");
            if (send_error_response(&socket_fd, "\"Table not exist.\"") < 0)
                printf("\033[0;31mError response failed.\033[0m\n");
            
            json_object_clear(json_object);
            json_value_free(json);
            free(http_parser);
            free(http_settings);
            free(socket);
            return 0;
        }
        
        if (http_parser->method == HTTP_GET) {
            int index = -1;
            char *search = NULL;

            if (json_object_has_value_of_type(json_object, "index", JSONNumber) != 0)
                index = (int)json_object_get_number(json_object, "index");
            if (json_object_has_value_of_type(json_object, "search", JSONArray) != 0)
                search = json_serialize_to_string(json_object_get_value(json_object, "search"));

            char *data = get_data(table_path, index, search);
            if (data == NULL) {
                printf("\033[0;31mDB request failed.\033[0m\n");
                if (send_error_response(&socket_fd, "\"DB request failed.\"") < 0)
                    printf("\033[0;31mError response failed.\033[0m\n");
                
                json_object_clear(json_object);
                json_value_free(json);
                free(http_parser);
                free(http_settings);
                free(socket);
                return 0;
            }
            if (send_success_response(&socket_fd, data) < 0)
                printf("\033[0;31mSuccess response failed.\033[0m\n");
        }
        else if (http_parser->method == HTTP_POST) {
            int index = -1;
            char *search = NULL;
            char *data = NULL;

            if (json_object_has_value_of_type(json_object, "index", JSONNumber) != 0)
                index = (int)json_object_get_number(json_object, "index");
            if (json_object_has_value_of_type(json_object, "search", JSONArray) != 0)
                search = json_serialize_to_string(json_object_get_value(json_object, "search"));
            if (json_object_has_value_of_type(json_object, "data", JSONArray) != 0)
                data = json_serialize_to_string(json_object_get_value(json_object, "data"));

            char *result = edit_data(table_path, index, search, data);
            if (result == NULL) {
                if (send_error_response(&socket_fd, "\"Edit failed.\"") < 0)
                    printf("\033[0;31mError response failed.\033[0m\n");
            }
            else {
                if (send_success_response(&socket_fd, result) < 0)
                    printf("\033[0;31mSuccess response failed.\033[0m\n"); 
            }
        }
        else if (http_parser->method == HTTP_PUT) {
            if (json_object_has_value_of_type(json_object, "row", JSONObject) == 0) {
                printf("\033[0;31mNo row to insert.\033[0m\n");
                if (send_error_response(&socket_fd, "\"Please fill `row` field.\"") < 0)
                    printf("\033[0;31mError response failed.\033[0m\n");
                
                json_object_clear(json_object);
                json_value_free(json);
                free(http_parser);
                free(http_settings);
                free(socket);
                return 0;
            }
            char *result = insert_data(table_path, json_serialize_to_string(json_object_get_value(json_object, "row")));
            if (result == NULL) {
                if (send_error_response(&socket_fd, "\"Insert failed.\"") < 0)
                    printf("\033[0;31mError response failed.\033[0m\n");
            }
            else {
                if (send_success_response(&socket_fd, result) < 0)
                    printf("\033[0;31mSuccess response failed.\033[0m\n"); 
            }
        }
        else if (http_parser->method == HTTP_DELETE) {
            int index = -1;
            char *search = NULL;

            if (json_object_has_value_of_type(json_object, "index", JSONNumber) != 0)
                index = (int)json_object_get_number(json_object, "index");
            if (json_object_has_value_of_type(json_object, "search", JSONArray) != 0)
                search = json_serialize_to_string(json_object_get_value(json_object, "search"));

            char *result = delete_data(table_path, index, search);
            if (result == NULL) {
                if (send_error_response(&socket_fd, "\"Delete failed.\"") < 0)
                    printf("\033[0;31mError response failed.\033[0m\n");
            }
            else {
                if (send_success_response(&socket_fd, result) < 0)
                    printf("\033[0;31mSuccess response failed.\033[0m\n"); 
            }
        }
        else {
            printf("\033[0;31mMethod not allowed.\033[0m\n");
            if (send_custom_response(&socket_fd, "HTTP/1.1 405 Method Not Allowed\nContent-Type: application/json\nContent-Length: 26\n\n\"This method not allowed.\"") < 0)
                printf("\033[0;31mCustom response failed.\033[0m\n");
        }
    }
    else {
        printf("\033[0;31mUnknown target.\033[0m\n");
        if (send_error_response(&socket_fd, "\"Unknown target.\"") < 0)
            printf("\033[0;31mError response failed.\033[0m\n");
    }

    json_object_clear(json_object);
    json_value_free(json);
    free(http_parser);
    free(http_settings);
    free(socket);
    return 0;
}

int
main(
    int argc, 
    char *argv[]
) {
    if (start_server(handler) < 1)
        printf("\033[0;31mServer start failed.\033[0m\n");
    return 0;
}