#include <obs-module.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/inotify.h>
#include <unistd.h>
#include <limits.h>
#include <pthread.h> 


#define BUF_LEN (10 * (sizeof(struct inotify_event) + NAME_MAX + 1))
 

struct watcher_source {
	uint32_t color;

	uint32_t width;
	uint32_t height;

	obs_source_t *src;
};
 
struct watcher_handler {
	void * src;
	void * cb;
};

 static void displayInotifyEvent(struct inotify_event *i)
 {
     printf("    wd =%2d; ", i->wd);
     if (i->cookie > 0)
         printf("cookie =%4d; ", i->cookie);
 
     printf("mask = ");
     if (i->mask & IN_ACCESS)        printf("IN_ACCESS ");
     if (i->mask & IN_ATTRIB)        printf("IN_ATTRIB ");
     if (i->mask & IN_CLOSE_NOWRITE) printf("IN_CLOSE_NOWRITE ");
     if (i->mask & IN_CLOSE_WRITE)   printf("IN_CLOSE_WRITE ");
     if (i->mask & IN_CREATE)        printf("IN_CREATE ");
     if (i->mask & IN_DELETE)        printf("IN_DELETE ");
     if (i->mask & IN_DELETE_SELF)   printf("IN_DELETE_SELF ");
     if (i->mask & IN_IGNORED)       printf("IN_IGNORED ");
     if (i->mask & IN_ISDIR)         printf("IN_ISDIR ");
     if (i->mask & IN_MODIFY)        printf("IN_MODIFY ");
     if (i->mask & IN_MOVE_SELF)     printf("IN_MOVE_SELF ");
     if (i->mask & IN_MOVED_FROM)    printf("IN_MOVED_FROM ");
     if (i->mask & IN_MOVED_TO)      printf("IN_MOVED_TO ");
     if (i->mask & IN_OPEN)          printf("IN_OPEN ");
     if (i->mask & IN_Q_OVERFLOW)    printf("IN_Q_OVERFLOW ");
     if (i->mask & IN_UNMOUNT)       printf("IN_UNMOUNT ");
     printf("\n");
 
     if (i->len > 0)
         printf("        name = %s\n", i->name);
 }

static void* watcher_source_watch(void * args)
{
	 int inotifyFd, wd, j;
     char buf[BUF_LEN] __attribute__ ((aligned(8)));
     ssize_t numRead;
     char *p;
     struct inotify_event *event;
	 struct watcher_source *context = args;
 
     inotifyFd = inotify_init();                 /* Create inotify instance */
     if (inotifyFd == -1)
         printf("[ERROR] inotify_init");
 
	 
     
	wd = inotify_add_watch(inotifyFd, "/tmp/test", IN_ALL_EVENTS);
	if (wd == -1)
		printf("[ERROR] inotify_add_watch");
 
    printf("Watching %s using wd %d\n", "/tmp/test", wd);
    
     for (;;) {                                  /* Read events forever */
         numRead = read(inotifyFd, buf, BUF_LEN);
         if (numRead == 0)
             printf("[ERROR] read() from inotify fd returned 0!");
 
         if (numRead == -1)
             continue;
 
         printf("Read %ld bytes from inotify fd\n", (long) numRead);
 
         /* Process all of the events in buffer returned by read() */
 
         for (p = buf; p < buf + numRead; ) {
             event = (struct inotify_event *) p;
             displayInotifyEvent(event);
			 context->color = 0xFFFFFF00;
             p += sizeof(struct inotify_event) + event->len;
         }
     }

}

static const char *watcher_source_get_name(void *unused)
{
	UNUSED_PARAMETER(unused);
	return obs_module_text("WatcherSource");
}

static void watcher_source_update(void *data, obs_data_t *settings)
{
	struct watcher_source *context = data;
	uint32_t color = (uint32_t)obs_data_get_int(settings, "color");
	uint32_t width = (uint32_t)obs_data_get_int(settings, "width");
	uint32_t height = (uint32_t)obs_data_get_int(settings, "height");

	context->color = color;
	context->width = width;
	context->height = height;

	pthread_t thread_id;
	pthread_create(&thread_id, NULL, watcher_source_watch, data); 

}

static void *watcher_source_create(obs_data_t *settings, obs_source_t *source)
{
	UNUSED_PARAMETER(source);

	struct watcher_source *context = bzalloc(sizeof(struct watcher_source));
	context->src = source;

	watcher_source_update(context, settings);

	return context;
}

static void watcher_source_destroy(void *data)
{
	bfree(data);
	//TODO: AG
   //inotify_rm_watch( fd, wd );
   //close( fd );
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
	.version = 2,
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
