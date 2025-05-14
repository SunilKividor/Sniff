#include <stdio.h>
#include <sys/inotify.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "../includes/file_watcher.h"
#include "../includes/server.h"

#define EVENT_BUF_LEN (1024 * (sizeof(struct inotify_event) + 16))

int init_file_watcher(const char *filePathName)
{
    if (!filePathName)
    {
        return -1;
    }
    return 1;
}

pthread_t start_file_watcher(void *args)
{
    pthread_t tid;
    pthread_create(&tid, NULL, watch_file, args);
    pthread_detach(tid);
    return tid;
}

void *watch_file(void *args)
{
    file_watcher_args_t watcher_args = *(file_watcher_args_t *)args;

    int fd = inotify_init();
    if (fd < 0)
    {
        perror("Error initiating inotify");
        return NULL;
    }

    int wd = inotify_add_watch(fd, watcher_args.filePath, IN_MODIFY);
    if (wd < 0)
    {
        perror("Error initiating inotify");
        close(fd);
        return NULL;
    }

    char buffer[EVENT_BUF_LEN];

    while (1)
    {
        int length = read(fd, buffer, EVENT_BUF_LEN);
        if (length < 0)
        {
            perror("read error");
            break;
        }

        int i = 0;
        while (i < length)
        {
            struct inotify_event *event = (struct inotify_event *)&buffer[i];
            if (event->mask & IN_MODIFY)
            {
                printf("[FILE WATCHER] file modified: %s\n", watcher_args.filePath);
                pthread_mutex_lock(watcher_args.mutex);
                *watcher_args.file_changed = 1;
                pthread_cond_signal(watcher_args.cond);
                pthread_mutex_unlock(watcher_args.mutex);
            }

            // Size of the inotify_event structure plus the length of the file name.
            // If watching a directory, event->len will be the length of the changed file's name.
            // If watching a single file directly, event->len will be 0.
            i += sizeof(struct inotify_event) + event->len;
        }
    }

    inotify_rm_watch(fd, wd);
    close(fd);
    return NULL;
}