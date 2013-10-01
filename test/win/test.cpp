#include <stdio.h>
#include <time.h>
#include <windows.h>

#include "util/base.h"
#include "obs.h"

static const int cx = 800;
static const int cy = 600;

/* --------------------------------------------------- */

class OBSContext {
	obs_t obs;

public:
	inline OBSContext(obs_t obs) : obs(obs) {}
	inline ~OBSContext() {obs_destroy(obs);}
	inline operator obs_t() {return obs;}
};

/* --------------------------------------------------- */

class SourceContext {
	source_t source;

public:
	inline SourceContext(source_t source) : source(source) {}
	inline ~SourceContext() {source_destroy(source);}
	inline operator source_t() {return source;}
};

/* --------------------------------------------------- */

class SceneContext {
	scene_t scene;

public:
	inline SceneContext(scene_t scene) : scene(scene) {}
	inline ~SceneContext() {scene_destroy(scene);}
	inline operator scene_t() {return scene;}
};

/* --------------------------------------------------- */

static LRESULT CALLBACK sceneProc(HWND hwnd, UINT message, WPARAM wParam,
		LPARAM lParam)
{
	switch (message) {

	case WM_CLOSE:
		PostQuitMessage(0);
		break;

	default:
		return DefWindowProc(hwnd, message, wParam, lParam);
	}

	return 0;
}

static void do_log(enum log_type type, const char *msg, va_list args)
{
	char bla[4096];
	vsnprintf(bla, 4095, msg, args);

	OutputDebugStringA(bla);
	OutputDebugStringA("\n");
}

static obs_t CreateOBS(HWND hwnd)
{
	struct video_info vi;
	memset(&vi, 0, sizeof(struct video_info));
	vi.format  = "RGBA";
	vi.fps_num = 30000;
	vi.fps_den = 1001;
	vi.width   = cx;
	vi.height  = cy;
	vi.name    = "video";

	struct gs_init_data gsid;
	memset(&gsid, 0, sizeof(gsid));
	gsid.hwnd            = hwnd;
	gsid.cx              = cx;
	gsid.cy              = cy;
	gsid.num_backbuffers = 2;
	gsid.format          = GS_RGBA;

	obs_t obs = obs_create("libobs-d3d11.dll", &gsid, &vi, NULL);
	if (!obs)
		throw "Couldn't create OBS";

	return obs;
}

static void AddTestItems(scene_t scene, source_t source)
{
	sceneitem_t item = NULL;
	struct vec2 v2;

	item = scene_add(scene, source);
	vec2_set(&v2, 100.0f, 200.0f);
	sceneitem_setpos(item, &v2);
	sceneitem_setrot(item, 10.0f);
	vec2_set(&v2, 20.0f, 2.0f);
	sceneitem_setscale(item, &v2);

	item = scene_add(scene, source);
	vec2_set(&v2, 200.0f, 100.0f);
	sceneitem_setpos(item, &v2);
	sceneitem_setrot(item, -45.0f);
	vec2_set(&v2, 5.0f, 7.0f);
	sceneitem_setscale(item, &v2);
}

static HWND CreateTestWindow(HINSTANCE instance)
{
	WNDCLASS wc;
	base_set_log_handler(do_log);

	memset(&wc, 0, sizeof(wc));
	wc.lpszClassName = L"bla";
	wc.hbrBackground = (HBRUSH)COLOR_WINDOW;
	wc.hInstance     = instance;
	wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
	wc.lpfnWndProc   = (WNDPROC)sceneProc;

	if (!RegisterClass(&wc))
		return 0;

	return CreateWindow(L"bla", L"bla", WS_OVERLAPPEDWINDOW|WS_VISIBLE,
			1920/2 - cx/2, 1080/2 - cy/2, cx, cy,
			NULL, NULL, instance, NULL);
}

/* --------------------------------------------------- */

int WINAPI WinMain(HINSTANCE instance, HINSTANCE prevInstance, LPSTR cmdLine,
		int numCmd)
{
	HWND hwnd = NULL;

	try {
		hwnd = CreateTestWindow(instance);
		if (!hwnd)
			throw "Couldn't create main window";

		/* ------------------------------------------------------ */
		/* create OBS */
		OBSContext obs = CreateOBS(hwnd);

		/* ------------------------------------------------------ */
		/* load module */
		if (obs_load_module(obs, "test-input.dll") != 0)
			throw "Couldn't load module";

		/* ------------------------------------------------------ */
		/* create source */
		SourceContext source = source_create(obs, SOURCE_INPUT,
				"random", NULL);
		if (!source)
			throw "Couldn't create random test source";

		/* ------------------------------------------------------ */
		/* create filter */
		SourceContext filter = source_create(obs, SOURCE_FILTER,
				"test", NULL);
		if (!filter)
			throw "Couldn't create test filter";
		source_filter_add(source, filter);

		/* ------------------------------------------------------ */
		/* create scene and add source to scene (twice) */
		SceneContext scene = scene_create(obs);
		if (!scene)
			throw "Couldn't create scene";

		AddTestItems(scene, source);

		/* ------------------------------------------------------ */
		/* set the scene as the primary draw source and go */
		obs_set_primary_source(obs, scene_source(scene));

		MSG msg;
		while (GetMessage(&msg, NULL, 0, 0)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		obs_set_primary_source(obs, NULL);

	} catch (char *error) {
		MessageBoxA(NULL, error, NULL, 0);
	}

	blog(LOG_INFO, "Number of memory leaks: %u", bnum_allocs());
	DestroyWindow(hwnd);

	return 0;
}
