#ifndef FILE_WATCHER_H
#define FILE_WATCHER_H

#include <pthread.h>

int init_file_watcher(const char* filePathName);
pthread_t start_file_watcher(void *args);
void* watch_file(void* args);

#endif