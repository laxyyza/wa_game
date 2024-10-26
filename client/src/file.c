#include "file.h"
#include <stdlib.h>
#include <stdio.h>

#ifdef _WIN32
#include <windows.h>
#elif __linux__
#include <dirent.h>
#endif

#define PATH_SIZE 512

#ifdef __linux__
array_t*
dir_files(const char* dir_path)
{
    array_t* array;
    struct dirent* dirent;
    DIR* dir;

    dir = opendir(dir_path);
    if (dir == NULL)
    {
        perror("opendir");
        return NULL;
    }

    array = calloc(1, sizeof(array_t));
    array_init(array, sizeof(const char*), 10);

    while ((dirent = readdir(dir)))
    {
        if (dirent->d_name[0] == '.')
            continue;

        char** path = array_add_into(array);
        *path = malloc(PATH_SIZE);
        snprintf(*path, PATH_SIZE, "%s/%s", dir_path, dirent->d_name);
    }

    closedir(dir);
    return array;
}
#endif // __linux__

#ifdef _WIN32
array_t*
dir_files(const char* _dir_path)
{
    u64 len = strlen(_dir_path) + 3;
    char* dir_path = malloc(len);
    snprintf(dir_path, len, "%s/*", _dir_path);

    array_t* array;
    WIN32_FIND_DATA find_file_data;
    HANDLE hfind = FindFirstFile(dir_path, &find_file_data);

    if (hfind == INVALID_HANDLE_VALUE)
    {
        fprintf(stderr, "dir_files: FindFirstFile() failed: %lX\n", 
                GetLastError());
        free(dir_path);
        return NULL;
    }
    array = calloc(1, sizeof(array_t));
    array_init(array, sizeof(char*), 5);

    do {
        const char* filename = find_file_data.cFileName;
        if (filename[0] == '.')
            continue;

        char** path = array_add_into(array);
        *path = malloc(PATH_SIZE);
        snprintf(*path, PATH_SIZE, "%s/%s", _dir_path, filename);
    } while (FindNextFile(hfind, &find_file_data));

    free(dir_path);
    FindClose(hfind);
    return array;
}
#endif // _WIN32
