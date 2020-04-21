#pragma once

char*
table_exist(
    const char *table_name
);

char*
get_data(
    char *table_path,
    int index,
    char *search
);

char*
insert_data(
    char *table_path,
    const char *row
);

char*
edit_data(
    char *table_path,
    int index,
    char *search,
    char *data
);

char*
delete_data(
    char *table_path,
    int index,
    char *search
);