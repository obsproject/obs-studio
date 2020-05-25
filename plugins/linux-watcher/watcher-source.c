#include <obs-module.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/inotify.h>
#include <sys/syscall.h>
#include <sys/queue.h>
#include <limits.h>
#include <pthread.h>
#include <unistd.h>

#include "obs-internal.h"

static pthread_mutex_t watcher_mutex = PTHREAD_MUTEX_INITIALIZER;
static signal_handler_t *watcher_signalhandler = NULL;
static const char *watcher_signals[] = {"void file_added(string file)", NULL};
#define BUF_LEN (10 * (sizeof(struct inotify_event) + NAME_MAX + 1))

//Queue for watcher files
typedef struct wfnode {
	char *file;
	TAILQ_ENTRY(wfnode) wfnodes;
} wfnode_t;
typedef TAILQ_HEAD(wfhead_s, wfnode) wfhead_t;

struct watcher_source {
	uint32_t color;

	uint32_t width;
	uint32_t height;

	pthread_t watcher;
	int wd;
	int wfd;
	wfhead_t wqh;

	obs_source_t *src;
};

static void file_added(void *data, calldata_t *calldata)
{
	//printf("before calling pthread_create getpid: %d getpthread_self: %lu\n",getpid(), pthread_self());
	printf("here failed ..\n");
	struct watcher_source *context = data;
	obs_source_update_properties(context->src);
	const char *file;
	calldata_get_string(calldata, "file", &file);
	struct wfnode *e = NULL;
	e = malloc(sizeof(struct wfnode));
	if (e == NULL) {
		fprintf(stderr, "malloc failed");
		exit(EXIT_FAILURE);
	}
	e->file = file;
	TAILQ_INSERT_TAIL(&context->wqh, e, wfnodes);
	// TAILQ_FOREACH(e, head, nodes)
    // {
    //     printf("%c", e->c);
    // }
	e = NULL;
	printf("booyah here %s\n", file);
}

static void *watcher_source_watch(void *data)
{
	char buf[BUF_LEN] __attribute__((aligned(8)));
	ssize_t numRead;
	char *p;
	struct inotify_event *event;
	struct watcher_source *context = data;

	context->wfd = inotify_init(); /* Create inotify instance */
	if (context->wfd == -1)
		printf("[ERROR] inotify_init");

	context->wd =
		inotify_add_watch(context->wfd, "/tmp/test", IN_ALL_EVENTS);
	if (context->wd == -1)
		printf("[ERROR] inotify_add_watch");

	printf("Watching %s using wd %d\n", "/tmp/test", context->wd);

	for (;;) { /* Read events forever */
		numRead = read(context->wfd, buf, BUF_LEN);
		if (numRead == 0)
			printf("[ERROR] read() from inotify fd returned 0!");

		if (numRead == -1)
			continue;

		/* Add a file to the queue if a file was created */
		for (p = buf; p < buf + numRead;) {
			event = (struct inotify_event *)p;
			if (event->mask & IN_CREATE) {
				// struct node *e = malloc(sizeof(struct node));
				// if (e == NULL) {
				// 	fprintf(stderr, "malloc failed");
				// 	exit(EXIT_FAILURE);
				// }

				printf("Found file %s\n", event->name);
				//TAILQ_INSERT_TAIL(&head, e, nodes);
				//e = NULL;
				struct calldata msg;

				pthread_mutex_lock(&watcher_mutex);

				calldata_init(&msg);
				char file[BUF_LEN];
				snprintf(file, sizeof(buf), "%s", event->name);
				calldata_set_string(&msg, "file", file);

				signal_handler_signal(watcher_signalhandler,
						      "file_added", &msg);

				calldata_free(&msg);

				pthread_mutex_unlock(&watcher_mutex);
			}
			context->color = 0xFFFFFF00;
			p += sizeof(struct inotify_event) + event->len;
		}
	}

	return NULL;
}

static const char *watcher_source_get_name(void *unused)
{
	UNUSED_PARAMETER(unused);
	return obs_module_text("WatcherSource");
}

static void watcher_clean(void *data)
{
	struct watcher_source *context = data;
	inotify_rm_watch(context->wfd, context->wd);
	close(context->wfd);
	while (!TAILQ_EMPTY(&context->wqh))
    {
        struct wfnode * e = TAILQ_FIRST(&context->wqh);
        TAILQ_REMOVE(&context->wqh, e, wfnodes);
        free(e);
        e = NULL;
    }
}

static void watcher_source_update(void *data, obs_data_t *settings)
{
	struct watcher_source *context = data;
	uint32_t color = (uint32_t)obs_data_get_int(settings, "color");
	uint32_t width = (uint32_t)obs_data_get_int(settings, "width");
	uint32_t height = (uint32_t)obs_data_get_int(settings, "height");

	if (context->watcher != (pthread_t)NULL) {
		pthread_cancel(context->watcher);
		pthread_join(context->watcher, NULL);
		watcher_clean(data);
	}
	context->color = color;
	context->width = width;
	context->height = height;
	pthread_create(&context->watcher, NULL, watcher_source_watch, context);
}

static void *watcher_source_create(obs_data_t *settings, obs_source_t *source)
{
	int x = syscall(SYS_gettid);
	//printf("%d",x);
	//printf("0 before calling pthread_create getpid: %d getpthread_self: %lu\n",getpid(), pthread_self());

	UNUSED_PARAMETER(source);

	struct watcher_source *context = bzalloc(sizeof(struct watcher_source));
	context->src = source;
	wfhead_t head;
	TAILQ_INIT(&head);
	context->wqh = head;

	pthread_mutex_lock(&watcher_mutex);	
	watcher_signalhandler = signal_handler_create();
	if (!watcher_signalhandler)
		goto fail;
	signal_handler_add_array(watcher_signalhandler, watcher_signals);

	signal_handler_connect(watcher_signalhandler, "file_added", &file_added,
			       context);

fail:
	pthread_mutex_unlock(&watcher_mutex);
	watcher_source_update(context, settings);

	return context;
}

static void watcher_source_destroy(void *data)
{
	bfree(data);
	watcher_clean(data);
}

static obs_properties_t *watcher_source_properties(void *unused)
{
	UNUSED_PARAMETER(unused);

	obs_properties_t *props = obs_properties_create();

	obs_properties_add_color(props, "color",
				 obs_module_text("WatcherSource.Color"));

	obs_properties_add_int(props, "width",
			       obs_module_text("WatcherSource.Width"), 0, 4096,
			       1);

	obs_properties_add_int(props, "height",
			       obs_module_text("WatcherSource.Height"), 0, 4096,
			       1);

	return props;
}

static void watcher_source_render(void *data, gs_effect_t *effect)
{
	UNUSED_PARAMETER(effect);

	struct watcher_source *context = data;

	gs_effect_t *solid = obs_get_base_effect(OBS_EFFECT_SOLID);
	gs_eparam_t *color = gs_effect_get_param_by_name(solid, "color");
	gs_technique_t *tech = gs_effect_get_technique(solid, "Solid");

	struct vec4 colorVal;
	vec4_from_rgba(&colorVal, context->color);
	gs_effect_set_vec4(color, &colorVal);

	gs_technique_begin(tech);
	gs_technique_begin_pass(tech, 0);

	gs_draw_sprite(0, 0, context->width, context->height);

	gs_technique_end_pass(tech);
	gs_technique_end(tech);
}

static uint32_t watcher_source_getwidth(void *data)
{
	struct watcher_source *context = data;
	return context->width;
}

static uint32_t watcher_source_getheight(void *data)
{
	struct watcher_source *context = data;
	return context->height;
}

static void watcher_source_defaults(obs_data_t *settings)
{
	obs_data_set_default_int(settings, "color", 0xFFFFFFFF);
	obs_data_set_default_int(settings, "width", 400);
	obs_data_set_default_int(settings, "height", 400);
}

struct obs_source_info watcher_source_info = {
	.id = "watcher_source",
	.version = 3,
	.type = OBS_SOURCE_TYPE_INPUT,
	.output_flags = OBS_SOURCE_VIDEO | OBS_SOURCE_CUSTOM_DRAW,
	.create = watcher_source_create,
	.destroy = watcher_source_destroy,
	.update = watcher_source_update,
	.get_name = watcher_source_get_name,
	.get_defaults = watcher_source_defaults,
	.get_width = watcher_source_getwidth,
	.get_height = watcher_source_getheight,
	.video_render = watcher_source_render,
	.get_properties = watcher_source_properties,
	.icon_type = OBS_ICON_TYPE_COLOR,
};
