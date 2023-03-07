/*
 * Copyright (c) 2013 Hugh Bailey <obs.jim@gmail.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include "obsconfig.h"
#include "obs-internal.h"

#if !defined(__APPLE__) && OBS_QT_VERSION == 6
#define _GNU_SOURCE
#include <link.h>
#include <stdlib.h>
#endif

#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <limits.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>

#if !defined(__APPLE__)
#include <sys/times.h>
#include <sys/wait.h>
#include <libgen.h>
#if defined(__FreeBSD__) || defined(__OpenBSD__)
#include <sys/param.h>
#include <sys/queue.h>
#include <sys/sysctl.h>
#include <sys/user.h>
#include <unistd.h>
#if defined(__FreeBSD__)
#include <libprocstat.h>
#endif
#else
#include <sys/resource.h>
#endif
#if !defined(__OpenBSD__)
#include <sys/sysinfo.h>
#endif
#include <spawn.h>
#endif

#if defined(__linux__)
#include <sys/inotify.h>
#endif

#include "darray.h"
#include "dstr.h"
#include "platform.h"
#include "threading.h"

static volatile long watch_counter = 0;

struct os_file_watchinfo {
        pthread_mutex_t mutex;
        pthread_t file_watch_thread;

#if defined(__linux__)
        int inotify_instance_fd;
        DARRAY(long)   inotify_obs_watch_ids;
        DARRAY(int)    inotify_watch_descriptors;
        DARRAY(char *) inotify_watched_paths;
        DARRAY(bool)   inotify_files_updated;
#endif

        DARRAY(long)     polling_obs_watch_ids;
        DARRAY(char *)   polling_watched_paths;
        DARRAY(time_t)   polling_path_timestamps;
        DARRAY(uint64_t) polling_last_checked;
        DARRAY(bool)     polling_try_upgrade_on_check;
};

void push_polling_watch(os_file_watchinfo_t *info,
                        long      watch_id,
                        char     *path,
                        time_t    timestamp,
                        uint64_t  check_time,
                        bool      try_upgrade)
{
        da_push_back(info->polling_obs_watch_ids,        &watch_id);
        da_push_back(info->polling_watched_paths,        &path);
        da_push_back(info->polling_path_timestamps,      &timestamp);
        da_push_back(info->polling_last_checked,         &check_time);
        da_push_back(info->polling_try_upgrade_on_check, &try_upgrade);
}

void erase_polling_watch(os_file_watchinfo_t *info, size_t idx)
{
        da_erase(info->polling_obs_watch_ids,        idx);
        da_erase(info->polling_watched_paths,        idx);
        da_erase(info->polling_path_timestamps,      idx);
        da_erase(info->polling_last_checked,         idx);
        da_erase(info->polling_try_upgrade_on_check, idx);
}

#if defined(__linux__)
#define INOTIFY_EVENT_MASK (IN_MODIFY | IN_CLOSE_WRITE | IN_DELETE_SELF)

void push_inotify_watch(os_file_watchinfo_t *info,
                        long  watch_id,
                        int   wd,
                        char *path)
{
        da_push_back(info->inotify_obs_watch_ids, &watch_id);
        da_push_back(info->inotify_watch_descriptors, &wd);
        da_push_back(info->inotify_watched_paths, &path);
        *(bool *)(da_push_back_new(info->inotify_files_updated)) = false;
}

void erase_inotify_watch(os_file_watchinfo_t *info, size_t idx)
{
        da_erase(info->inotify_obs_watch_ids,     idx);
        da_erase(info->inotify_watch_descriptors, idx);
        da_erase(info->inotify_watched_paths,     idx);
        da_erase(info->inotify_files_updated,     idx);
}

// When rewatching, we will revert to polling mode if the rewatch
// fails for any reason, in contrast to creating a new inotify watch
// where we just fail.
void inotify_rewatch(os_file_watchinfo_t *info, int wd_idx)
{
        int new_watch, old_watch;
        long watch_id;
        char *watch_path;
        time_t polling_time;
        uint64_t polling_check_time;

        old_watch = info->inotify_watch_descriptors.array[wd_idx];
        inotify_rm_watch(info->inotify_instance_fd, old_watch);

        new_watch = inotify_add_watch(info->inotify_instance_fd,
                                      info->inotify_watched_paths.array[wd_idx],
                                      INOTIFY_EVENT_MASK);

        // We can have multiple watches on the same file, so we have to be sure
        // to replace all of the descriptors referring to the same watch.

        if (new_watch != -1) {
                for (size_t next = wd_idx;
                     next != DARRAY_INVALID;
                     next = da_find(info->inotify_watch_descriptors,
                                    &old_watch,
                                    next + 1)) {
                        info->inotify_watch_descriptors.array[next] = new_watch;
                }
                return;
        }

        // Fall back to polling for all old files
        // Do note that this thread could terminate on the calls to time() and
        // os_gettime_ns()

        polling_time = time(NULL);
        polling_check_time = os_gettime_ns();

        for (size_t next = wd_idx;
             next != DARRAY_INVALID;
             next = da_find(info->inotify_watch_descriptors,
                            &old_watch,
                            0)) {
                push_polling_watch(info,
                                   info->inotify_obs_watch_ids.array[next],
                                   info->inotify_watched_paths.array[next],
                                   polling_time,
                                   polling_check_time,
                                   true);

                erase_inotify_watch(info, next);
        }
}

void *os_file_watch_thread(void *param)
{
#define MAX_INOTIFY_EVENT_LENGTH (sizeof(struct inotify_event) + NAME_MAX + 1)
        char inotify_buffer[8*MAX_INOTIFY_EVENT_LENGTH];
        size_t bytes_read;
        char *buffer_view;
        struct inotify_event *ev;

        size_t wd_idx;

        os_file_watchinfo_t *info = obs->data.file_watchinfo;

        while (true)
        {

        bytes_read = read(info->inotify_instance_fd,
                          inotify_buffer,
                          8*MAX_INOTIFY_EVENT_LENGTH);

        if (bytes_read <= 0) {
                blog(LOG_ERROR, "Inotify read failed. Exiting watch thread.");
                pthread_mutex_lock(&info->mutex);
                close(info->inotify_instance_fd);
                info->inotify_instance_fd = -1;
                pthread_mutex_unlock(&info->mutex);
                pthread_exit(NULL);
        }

        pthread_mutex_lock(&info->mutex);

        for (buffer_view = inotify_buffer;
             buffer_view < inotify_buffer + bytes_read; ) {
                ev = (struct inotify_event *) buffer_view;
                wd_idx = da_find(info->inotify_watch_descriptors, &ev->wd, 0);

                if (wd_idx == DARRAY_INVALID) {
                        goto next_iter;
                }

                if (ev->mask & IN_DELETE_SELF) {
                        inotify_rewatch(info, wd_idx);
                } else {
                        for (size_t next = wd_idx;
                             next != DARRAY_INVALID;
                             next = da_find(info->inotify_watch_descriptors,
                                            &ev->wd,
                                            next + 1)) {
                                info->inotify_files_updated.array[next] = true;
                        }
                }

                next_iter:
                buffer_view += sizeof(struct inotify_event) + ev->len;
        }

        pthread_mutex_unlock(&info->mutex);

        }

        UNUSED_PARAMETER(param);
}
#endif


void os_initialize_file_watching(void)
{
        os_file_watchinfo_t *info = bzalloc(sizeof(struct os_file_watchinfo));

        if (pthread_mutex_init_recursive(&info->mutex) != 0) {
                bcrash("Failed to initialize mutex for file watching.");
        }

        da_init(info->polling_obs_watch_ids);
        da_init(info->polling_watched_paths);
        da_init(info->polling_path_timestamps);
        da_init(info->polling_last_checked);
        da_init(info->polling_try_upgrade_on_check);

        obs->data.file_watchinfo = info;

#if defined(__linux__)
        if ((info->inotify_instance_fd = inotify_init()) == -1) {
                blog(LOG_ERROR, "Could not instantiate inotify. "
                                "Check your system limits to ensure you are "
                                "not exceeding the maximum number of "
                                "instances. Falling back to polling.");
                return;
        }

        da_init(info->inotify_obs_watch_ids);
        da_init(info->inotify_watch_descriptors);
        da_init(info->inotify_watched_paths);
        da_init(info->inotify_files_updated);

        if (pthread_create(&info->file_watch_thread, NULL,
                           os_file_watch_thread, obs) != 0) {
                blog(LOG_ERROR, "Could not start inotify watch thread. "
                                "Falling back to polling.");
                close(info->inotify_instance_fd);
                info->inotify_instance_fd = -1;
                da_free(info->inotify_obs_watch_ids);
                da_free(info->inotify_watch_descriptors);
                da_free(info->inotify_watched_paths);
                da_free(info->inotify_files_updated);
        }
#endif
}

void os_destroy_file_watchinfo(os_file_watchinfo_t *info)
{
#if defined(__linux__)
        pthread_cancel(info->file_watch_thread);
        pthread_join(info->file_watch_thread, NULL);
#endif

        pthread_mutex_destroy(&info->mutex);

        da_free(info->polling_obs_watch_ids);
        for (size_t i = 0; i < info->polling_watched_paths.num; i++) {
                bfree(info->polling_watched_paths.array[i]);
        }
        da_free(info->polling_watched_paths);
        da_free(info->polling_path_timestamps);
        da_free(info->polling_last_checked);
        da_free(info->polling_try_upgrade_on_check);

#if defined(__linux__)
        if (info->inotify_instance_fd != -1) {
                close(info->inotify_instance_fd);
                info->inotify_instance_fd = -1;

                da_free(info->inotify_obs_watch_ids);
                da_free(info->inotify_watch_descriptors);
                for (size_t i = 0; i < info->inotify_watched_paths.num; i++) {
                        bfree(info->inotify_watched_paths.array[i]);
                }
                da_free(info->inotify_watched_paths);
                da_free(info->inotify_files_updated);
        }
#endif

        bfree(info);
}

os_file_watch_t os_watch_file_polling(const char *path)
{

        os_file_watch_t watch = UNINITIALIZED_WATCH;

        os_file_watchinfo_t *info = obs->data.file_watchinfo;
        uint64_t check_time;
        char *path_copy;
        long watch_id;
        struct stat stats;

        if (!path || !*path) {
                return watch;
        }

        watch_id = os_atomic_inc_long(&watch_counter);

	if (os_stat(path, &stats) != 0) {
		return watch;
        }

        path_copy = bstrdup(path);

        pthread_mutex_lock(&info->mutex);

        check_time = os_gettime_ns();
        push_polling_watch(info,
                           watch_id, path_copy,
                           stats.st_mtime, check_time,
                           false);

        pthread_mutex_unlock(&info->mutex);

        watch.watch_id= watch_id;
        watch.watch_flags = POLLING_WATCH;
        return watch;
}

#if defined(__linux__)
os_file_watch_t os_watch_file_inotify(const char *path)
{
        os_file_watch_t watch = UNINITIALIZED_WATCH; 
        os_file_watchinfo_t *info = obs->data.file_watchinfo;
        long watch_id;
        char *path_copy;
        int wd;

        if (!path || !*path) {
                return watch;
        }

        watch_id = os_atomic_inc_long(&watch_counter);

        path_copy = bstrdup(path);

        pthread_mutex_lock(&info->mutex);

        wd = inotify_add_watch(info->inotify_instance_fd,
                               path,
                               INOTIFY_EVENT_MASK);
        if (wd == -1) {
                bfree(path_copy);
                return watch;
        }

        push_inotify_watch(info, watch_id, wd, path_copy);

        pthread_mutex_unlock(&info->mutex);

        watch.watch_id = watch_id;
        watch.watch_flags = INOTIFY_WATCH;
        return watch;
}
#endif

os_file_watch_t os_watch_file(const char *path)
{
#if defined(__linux__)
        if (obs->data.file_watchinfo->inotify_instance_fd != -1) {
                return os_watch_file_inotify(path);
        }
#endif
        return os_watch_file_polling(path);
}

void os_remove_file_watch_polling(os_file_watchinfo_t *info, long watch_id)
{
        size_t idx = da_find(info->polling_obs_watch_ids, &watch_id, 0);

        if (idx == DARRAY_INVALID) {
                return;
        }

        bfree(info->polling_watched_paths.array[idx]);
        erase_polling_watch(info, idx);
}

#if defined(__linux__)
void os_remove_file_watch_inotify(os_file_watchinfo_t *info, long watch_id)
{
        size_t idx = da_find(info->inotify_obs_watch_ids, &watch_id, 0);

        if (idx == DARRAY_INVALID) {
                // Try removing polling fallbacks
                os_remove_file_watch_polling(info, watch_id);
                return;
        }

        if (da_find(info->inotify_watch_descriptors,
                    &info->inotify_watch_descriptors.array[idx],
                    idx + 1) == DARRAY_INVALID) {
                // Only remove the watch if it's the last one in the list

                inotify_rm_watch(info->inotify_instance_fd,
                                 info->inotify_watch_descriptors.array[idx]);
        }

        bfree(info->inotify_watched_paths.array[idx]);

        erase_inotify_watch(info, idx);
}
#endif

void os_remove_file_watch(os_file_watch_t watch)
{
        os_file_watchinfo_t *info = obs->data.file_watchinfo;

        if (watch.watch_flags == INVALID_WATCH) {
                return;
        }

        pthread_mutex_lock(&info->mutex);

#if defined(__linux__)
        if (watch.watch_flags == INOTIFY_WATCH) {
                os_remove_file_watch_inotify(info, watch.watch_id);
        }
#endif

        if (watch.watch_flags == POLLING_WATCH) {
                os_remove_file_watch_polling(info, watch.watch_id);
        }

        pthread_mutex_unlock(&info->mutex);

}

#if defined(__linux__)
bool os_check_watch_for_updates_inotify(os_file_watchinfo_t *info,
                                        os_file_watch_t watch)
{
        size_t idx;

        idx = da_find(info->inotify_obs_watch_ids, &watch.watch_id, 0);
        if (idx == DARRAY_INVALID) {
                return false;
        }

        if (info->inotify_files_updated.array[idx]) {
                info->inotify_files_updated.array[idx] = false;
                return true;
        }

        return false;
}
#endif

void os_try_upgrade_polling_watch(os_file_watchinfo_t *info,
                                  os_file_watch_t watch,
                                  size_t idx)
{
#if defined(__linux__)
        int new_watch;

        if (watch.watch_flags != INOTIFY_WATCH) {
                return;
        }

        new_watch = inotify_add_watch(info->inotify_instance_fd,
                                      info->polling_watched_paths.array[idx],
                                      INOTIFY_EVENT_MASK);

        if (new_watch == -1) {
                return;
        }

        push_inotify_watch(info,
                           watch.watch_id, new_watch,
                           info->polling_watched_paths.array[idx]);

        erase_polling_watch(info, idx);

#endif
}

bool os_check_watch_for_updates_polling(os_file_watchinfo_t *info,
                                        os_file_watch_t watch)
{
        size_t idx;
        uint64_t check_time;

        struct stat st;

        idx = da_find(info->polling_obs_watch_ids, &watch.watch_id, 0);
        if (idx == DARRAY_INVALID) {
                return false;
        }
        check_time = os_gettime_ns();
        if (check_time - info->polling_last_checked.array[idx] >= 1000000000 ) {
                info->polling_last_checked.array[idx] = check_time;

	        if (os_stat(info->polling_watched_paths.array[idx], &st) != 0) {
		        return false;
                }

                if (st.st_mtime > info->polling_path_timestamps.array[idx]) {
                        info->polling_path_timestamps.array[idx] = st.st_mtime;
                        if (info->polling_try_upgrade_on_check.array[idx]) {
                                os_try_upgrade_polling_watch(info, watch, idx);
                        }
                        return true;
                }

                return false;
        }

        return false;
}

bool os_check_watch_for_updates(os_file_watch_t watch)
{
        os_file_watchinfo_t *info = obs->data.file_watchinfo;
        bool update_found = false;

        if (watch.watch_flags == INVALID_WATCH) {
                return false;
        }

        pthread_mutex_lock(&info->mutex);

#if defined(__linux__)
        if (watch.watch_flags == INOTIFY_WATCH) {
                update_found = os_check_watch_for_updates_inotify(info, watch);
        }
#endif

        if (!update_found) {
                update_found = os_check_watch_for_updates_polling(info, watch);
        }

        pthread_mutex_unlock(&info->mutex);

        return update_found;
}

