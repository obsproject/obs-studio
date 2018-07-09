#include <stdlib.h>
#include <util/threading.h>
#include <util/platform.h>
#include <obs.h>
#include <obs-module.h>
#include <gst/gst.h> 
#include <glib.h> 
#include <gst/app/gstappsink.h>

OBS_DECLARE_MODULE()

struct gst_tex {
	obs_source_t *source;
	os_event_t   *stop_signal;
	pthread_t    thread;
	bool         initialized;
};

static const char *gst_getname(void *unused)
{
	UNUSED_PARAMETER(unused);
	return "GStreamer OBS Plugin";
}

static void gst_destroy(void *data)
{
	struct gst_tex *rt = data;

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
	GstElement *pipeline;
	GstElement *appsink;
	GstBus *bus;
	GstMessage *msg;
	GstSample *gstSample;
	GstBuffer *gstBuf;
	GstMapInfo map;
	GstCaps *caps;
    GstStructure *s;
	gint width, height;
	char argc, argv;
	gboolean res;


	struct gst_tex   *rt = data;
	uint8_t  pixels[720000];
	uint64_t cur_time = os_gettime_ns();

	struct obs_source_frame frame = {
		.data     = {[0] = (uint8_t *)pixels},
		.linesize[0] = 800*3,
		.width    = 800,
		.height   = 600,
		.format   = VIDEO_FORMAT_BGRX
	};

	gst_init (&argc, &argv);
	pipeline = gst_parse_launch ("videotestsrc pattern=18 ! videoconvert ! video/x-raw,fornat=BGRx,width=800,height=600 ! queue ! appsink name=sink ", NULL);

	appsink = gst_bin_get_by_name (GST_BIN (pipeline), "sink");
	//g_object_set (appsink, "sync", TRUE, NULL); 
    //g_object_set (appsink, "emit-signals", TRUE, NULL);
    ///g_signal_connect (appsink , "new-sample", G_CALLBACK (new_sample),pipeline);

  	//Start playing 
	gst_element_set_state (pipeline, GST_STATE_PLAYING);

	
	bus = gst_element_get_bus (pipeline);

	while (os_event_try(rt->stop_signal) == EAGAIN) {
		//fill_texture(pixels);
		blog(LOG_INFO, "GST Sample: waiting");
		gstSample = gst_app_sink_pull_sample(appsink);
		blog(LOG_INFO, "GST Sample: have");
		caps = gst_sample_get_caps (gstSample);
		s = gst_caps_get_structure (caps, 0);
		res = gst_structure_get_int (s, "width", &width);
		res |= gst_structure_get_int (s, "height", &height);
		blog(LOG_INFO, "GST Sample: width: %d",width);
		blog(LOG_INFO, "GST Sample: height: %d", height);
		
		gstBuf = gst_sample_get_buffer(gstSample);
		if (gst_buffer_extract(gstBuf, 0, &pixels, 720000) > 0) {
		//if (gst_buffer_map (gstBuf, &map, GST_MAP_READ)) {
			blog(LOG_INFO, "GST Sample: address %u", map.data);
			blog(LOG_INFO, "GST Sample: datasize: %u", map.size);
			//pixels = *map.data;
			blog(LOG_INFO, "copying %d bytes from %u to %u",map.size, map.data,*pixels);
			//memcpy(pixels,&map.data,map.size);
			frame.timestamp = cur_time;
			
			obs_source_output_video(rt->source, &frame);
			//gst_buffer_unmap (gstBuf, &map);
		}
		gst_sample_unref (gstSample);
		os_sleepto_ns(cur_time += 250000000);
	}

	gst_element_set_state (pipeline, GST_STATE_NULL);
	gst_object_unref (pipeline);

	return NULL;
}
/*
static GstFlowReturn new_sample (GstElement *appsink, GstElement *data) 
{
     GstSample *sample = NULL;
	blog(LOG_DEBUG,"sample received");
	 GstElement *piappsink = gst_bin_get_by_name (GST_BIN (data), "sink");
     // Retrieve the buffer 
     g_signal_emit_by_name (piappsink, "pull-sample", &sample,NULL);
     if (sample) 
     {
          g_print ("*");
          gst_sample_unref (sample);
     }
}
*/
/*static void gst_source_activate(void *data)
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

  	/* Start playing 
	gst_element_set_state (pipeline, GST_STATE_PLAYING);

	
	bus = gst_element_get_bus (pipeline);
	/* Wait until error or EOS 
	msg = gst_bus_timed_pop_filtered (bus, GST_CLOCK_TIME_NONE, GST_MESSAGE_ERROR | GST_MESSAGE_EOS);

	// Free resources 
	if (msg != NULL)
		gst_message_unref (msg);
	gst_object_unref (bus);
	gst_element_set_state (pipeline, GST_STATE_NULL);
	gst_object_unref (pipeline);
	
	return;
}
*/

static void *gst_create(obs_data_t *settings, obs_source_t *source)
{
	
	struct gst_tex *rt = bzalloc(sizeof(struct gst_tex));
	rt->source = source;

	if (os_event_init(&rt->stop_signal, OS_EVENT_TYPE_MANUAL) != 0) {
		gst_destroy(rt);
		return NULL;
	}

	if (pthread_create(&rt->thread, NULL, video_thread, rt) != 0) {
		gst_destroy(rt);
		return NULL;
	}

	rt->initialized = true;

	UNUSED_PARAMETER(settings);
	UNUSED_PARAMETER(source);
	return rt;
}

struct obs_source_info gst_plugin = {
	.id           = "gst-plugin",
	.type         = OBS_SOURCE_TYPE_INPUT,
	.output_flags = OBS_SOURCE_ASYNC_VIDEO,
	.get_name     = gst_getname,
	.create       = gst_create,
	.destroy      = gst_destroy
};


bool obs_module_load(void)
{
	obs_register_source(&gst_plugin);
}