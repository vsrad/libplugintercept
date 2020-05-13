#include "fs_utils.hpp"

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

void agent::fs_utils::create_parent_directories(const char* path)
{
    char* path_buf = strdup(path);

    char* next_dir_sep = path_buf;
    while (1) {
        next_dir_sep = strchr(next_dir_sep + 1, '/');
        if (next_dir_sep == NULL)
            break;
        *next_dir_sep = '\0'; // path_buf now contains a null-terminated path to the base directory

        if (mkdir(path_buf, 0777) != 0 && errno != EEXIST)
            break;

        *next_dir_sep = '/';
    }

    free(path_buf);
}
