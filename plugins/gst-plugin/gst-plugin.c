#include <stdlib.h>
#include <util/threading.h>
#include <util/platform.h>
#include <obs.h>
#include <obs-module.h>
#include <gst/gst.h> 
#include <glib.h> 



OBS_DECLARE_MODULE()

struct my_tex {
	obs_source_t *source;
	os_event_t   *stop_signal;
	pthread_t    thread;
	bool         initialized;
};

static const char *my_getname(void *unused)
{
	UNUSED_PARAMETER(unused);
	return "GStreamer OBS Plugin";
}

static void my_destroy(void *data)
{
	struct my_tex *rt = data;

	if (rt) {
		if (rt->initialized) {
			os_event_signal(rt->stop_signal);
			pthread_join(rt->thread, NULL);
		}

		os_event_destroy(rt->stop_signal);
		bfree(rt);
	}
}

static inline void fill_texture(uint32_t *pixels)
{
	size_t x, y;

	for (y = 0; y < 20; y++) {
		for (x = 0; x < 20; x++) {
			uint32_t pixel = 0;
			pixel |= (rand()%256);
			pixel |= (rand()%256) << 8;
			pixel |= (rand()%256) << 16;
			//pixel |= (rand()%256) << 24;
			//pixel |= 0xFFFFFFFF;
			pixels[y*20 + x] = pixel;
		}
	}
}

static void *video_thread(void *data)
{
	struct my_tex   *rt = data;
	uint32_t            pixels[20*20];
	uint64_t            cur_time = os_gettime_ns();

	struct obs_source_frame frame = {
		.data     = {[0] = (uint8_t*)pixels},
		.linesize = {[0] = 20*4},
		.width    = 20,
		.height   = 20,
		.format   = VIDEO_FORMAT_BGRX
	};

	while (os_event_try(rt->stop_signal) == EAGAIN) {
		fill_texture(pixels);

		frame.timestamp = cur_time;

		obs_source_output_video(rt->source, &frame);

		os_sleepto_ns(cur_time += 250000000);
	}

	return NULL;
}

static GstFlowReturn new_sample (GstElement *appsink, GstElement *data) 
{
     GstSample *sample = NULL;
	blog(LOG_DEBUG,"sample received");
	 GstElement *piappsink = gst_bin_get_by_name (GST_BIN (data), "sink");
     /* Retrieve the buffer */
     g_signal_emit_by_name (piappsink, "pull-sample", &sample,NULL);
     if (sample) 
     {
          g_print ("*");
          gst_sample_unref (sample);
     }
}

static void my_source_activate(void *data)
{
	struct gst_plugin *rt = data;

	GstElement *pipeline;
	GstElement *appsink;
	GstBus *bus;
	GstMessage *msg;
	char argc, argv;
	gst_init (&argc, &argv);
	pipeline = gst_parse_launch ("videotestsrc ! autovideosink ", NULL);

	appsink = gst_bin_get_by_name (GST_BIN (pipeline), "sink");
	g_object_set (appsink, "sync", TRUE, NULL); 
    g_object_set (appsink, "emit-signals", TRUE, NULL);
    g_signal_connect (appsink , "new-sample", G_CALLBACK (new_sample),pipeline);

  	/* Start playing */
	gst_element_set_state (pipeline, GST_STATE_PLAYING);

	
	bus = gst_element_get_bus (pipeline);
	/* Wait until error or EOS */
	//msg = gst_bus_timed_pop_filtered (bus, GST_CLOCK_TIME_NONE, GST_MESSAGE_ERROR | GST_MESSAGE_EOS);

	/* Free resources 
	if (msg != NULL)
		gst_message_unref (msg);
	gst_object_unref (bus);
	gst_element_set_state (pipeline, GST_STATE_NULL);
	gst_object_unref (pipeline);
	*/
	return;
}




static void *my_create(obs_data_t *settings, obs_source_t *source)
{
	
	UNUSED_PARAMETER(settings);
	UNUSED_PARAMETER(source);
	return;
}

struct obs_source_info gst_plugin = {
	.id           = "gst-plugin",
	.type         = OBS_SOURCE_TYPE_INPUT,
	.output_flags = OBS_SOURCE_ASYNC_VIDEO,
	.get_name     = my_getname,
	.create       = my_create,
	.destroy      = my_destroy,
	.activate       = my_source_activate,
};


bool obs_module_load(void)
{
	obs_register_source(&gst_plugin);
}