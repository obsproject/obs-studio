#include <stdio.h>
#include <time.h>

#include <QtWidgets>
#include <QX11Info>

#include <util/base.h>
#include <obs.h>

static const int cx = 800;
static const int cy = 600;

/* --------------------------------------------------- */

class SourceContext {
	obs_source_t *source;

public:
	inline SourceContext(obs_source_t *source) : source(source) {}
	inline ~SourceContext() {obs_source_release(source);}
	inline operator obs_source_t*() {return source;}
};

/* --------------------------------------------------- */

class SceneContext {
	obs_scene_t *scene;

public:
	inline SceneContext(obs_scene_t *scene) : scene(scene) {}
	inline ~SceneContext() {obs_scene_release(scene);}
	inline operator obs_scene_t*() {return scene;}
};

/* --------------------------------------------------- */

class DisplayContext {
	obs_display_t *display;

public:
	inline DisplayContext(obs_display_t *display) : display(display) {}
	inline ~DisplayContext() {obs_display_destroy(display);}
	inline operator obs_display_t*() {return display;}
};

/* --------------------------------------------------- */

static void CreateOBS(QWidget *widget)
{
	QSize size = widget->size();

	if (!obs_startup("en-US", nullptr, nullptr))
		throw "Couldn't create OBS";

	struct obs_video_info ovi;
	ovi.adapter         = 0;
	ovi.base_width      = size.width();
	ovi.base_height     = size.height();
	ovi.fps_num         = 30000;
	ovi.fps_den         = 1001;
	ovi.graphics_module = DL_OPENGL;
	ovi.output_format   = VIDEO_FORMAT_RGBA;
	ovi.output_width    = size.width();
	ovi.output_height   = size.height();

	if (obs_reset_video(&ovi) != 0)
		throw "Couldn't initialize video";
}

static DisplayContext CreateDisplay(QWidget *widget)
{
	QSize size = widget->size();

	gs_init_data info = {};
	info.cx = size.width();
	info.cy = size.height();
	info.format = GS_RGBA;
	info.zsformat = GS_ZS_NONE;
	info.window.id = widget->winId();
	info.window.display = QX11Info::display();

	return obs_display_create(&info, 0);
}

static void AddTestItems(obs_scene_t *scene, obs_source_t *source)
{
	obs_sceneitem_t *item = NULL;
	struct vec2 scale;

	vec2_set(&scale, 20.0f, 20.0f);

	item = obs_scene_add(scene, source);
	obs_sceneitem_set_scale(item, &scale);
}

/* --------------------------------------------------- */

static void RenderWindow(void *data, uint32_t cx, uint32_t cy)
{
	obs_render_main_texture();

	UNUSED_PARAMETER(data);
	UNUSED_PARAMETER(cx);
	UNUSED_PARAMETER(cy);
}

/* --------------------------------------------------- */

int main(int argc, char* argv[])
{
	QApplication app(argc, argv);

	try {
    	QWidget *widget = new QWidget();
		if (!widget)
			throw "Couldn't create widget";

		widget->resize(cx, cy);

		/* ------------------------------------------------------ */
		/* create OBS */
		CreateOBS(widget);

		/* ------------------------------------------------------ */
		/* load modules */
		obs_load_all_modules();

		/* ------------------------------------------------------ */
		/* create source */
		SourceContext source = obs_source_create("random",
				"some randon source", NULL, nullptr);
		if (!source)
			throw "Couldn't create random test source";

		/* ------------------------------------------------------ */
		/* create filter */
		SourceContext filter = obs_source_create("test_filter",
				"a nice green filter", NULL, nullptr);
		if (!filter)
			throw "Couldn't create test filter";
		obs_source_filter_add(source, filter);

		/* ------------------------------------------------------ */
		/* create scene and add source to scene (twice) */
		SceneContext scene = obs_scene_create("test scene");
		if (!scene)
			throw "Couldn't create scene";

		AddTestItems(scene, source);

		/* ------------------------------------------------------ */
		/* set the scene as the primary draw source and go */
		obs_set_output_source(0, obs_scene_get_source(scene));

		/* ------------------------------------------------------ */
		/* create display for output and set the output render callback */
		DisplayContext display = CreateDisplay(widget);
		obs_display_add_draw_callback(display, RenderWindow, nullptr);

		widget->show();

		app.exec();
	} catch (const char *error) {
		QMessageBox::critical(nullptr, "", error);
	}

	obs_shutdown();

	blog(LOG_INFO, "Number of memory leaks: %ld", bnum_allocs());

	return 0;
}
