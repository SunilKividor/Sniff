#ifndef FILE_WATCHER_H
#define FILE_WATCHER_H

#include <pthread.h>

typedef struct {
    char* filePathName;
    bool* file_changed;
} watch_file_args_t;

int init_file_watcher(const char* filePathName);
pthread_t start_file_watcher(const char* filePathName);
void* watch_file(void* args);

#endif