#include <glad/glad.h>
#include <glad/glad_glx.h>
#include <X11/Xlib.h>
#include <X11/extensions/Xcomposite.h>
#include <pthread.h>

#include <vector>

#include <obs-module.h>
#include <graphics/vec4.h>
#include <util/platform.h>

#include "xcompcap-main.h"
#include "xcompcap-helper.h"

#define xdisp (XCompcap::disp())
#define WIN_STRING_DIV "\r\n"

bool XCompcapMain::init()
{
	if (!xdisp) {
		blog(LOG_ERROR, "failed opening display");
		return false;
	}

	int eventBase, errorBase;
	if (!XCompositeQueryExtension(xdisp, &eventBase, &errorBase)) {
		blog(LOG_ERROR, "Xcomposite extension not supported");
		return false;
	}

	int major = 0, minor = 2;
	XCompositeQueryVersion(xdisp, &major, &minor);

	if (major == 0 && minor < 2) {
		blog(LOG_ERROR, "Xcomposite extension is too old: %d.%d < 0.2",
				major, minor);
		return false;
	}

	return true;
}

void XCompcapMain::deinit()
{
	XCompcap::cleanupDisplay();
}

obs_properties_t XCompcapMain::properties()
{
	obs_properties_t props = obs_properties_create();

	obs_property_t wins = obs_properties_add_list(props, "capture_window",
			obs_module_text("Window"),
			OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);

	for (Window win: XCompcap::getTopLevelWindows()) {
		std::string wname = XCompcap::getWindowName(win);
		std::string progpath = XCompcap::getWindowCommand(win);
		std::string winid = std::to_string((long long)win);
		std::string desc =
			(winid + WIN_STRING_DIV + wname +
			 WIN_STRING_DIV + progpath);

		obs_property_list_add_string(wins, wname.c_str(),
				desc.c_str());
	}

	obs_properties_add_int(props, "cut_top", obs_module_text("CropTop"),
			0, 4096, 1);
	obs_properties_add_int(props, "cut_left", obs_module_text("CropLeft"),
			0, 4096, 1);
	obs_properties_add_int(props, "cut_right", obs_module_text("CropRight"),
			0, 4096, 1);
	obs_properties_add_int(props, "cut_bot", obs_module_text("CropBottom"),
			0, 4096, 1);

	obs_properties_add_bool(props, "swap_redblue",
			obs_module_text("SwapRedBlue"));
	obs_properties_add_bool(props, "lock_x", obs_module_text("LockX"));

	return props;
}

void XCompcapMain::defaults(obs_data_t settings)
{
	obs_data_set_default_string(settings, "capture_window", "");
	obs_data_set_default_int(settings, "cut_top", 0);
	obs_data_set_default_int(settings, "cut_left", 0);
	obs_data_set_default_int(settings, "cut_right", 0);
	obs_data_set_default_int(settings, "cut_bot", 0);
	obs_data_set_default_bool(settings, "swap_redblue", false);
	obs_data_set_default_bool(settings, "lock_x", false);
}


struct XCompcapMain_private
{
	XCompcapMain_private()
		:win(0)
		,cut_top(0), cur_cut_top(0)
		,cut_left(0), cur_cut_left(0)
		,cut_right(0), cur_cut_right(0)
		,cut_bot(0), cur_cut_bot(0)
		,inverted(false)
		,width(0),height(0)
		,pixmap(0)
		,glxpixmap(0)
		,tex(0)
		,gltex(0)
	{
		pthread_mutexattr_init(&lockattr);
		pthread_mutexattr_settype(&lockattr, PTHREAD_MUTEX_RECURSIVE);

		pthread_mutex_init(&lock, &lockattr);
	}

	~XCompcapMain_private()
	{
		pthread_mutex_destroy(&lock);
		pthread_mutexattr_destroy(&lockattr);
	}

	obs_source_t source;

	Window win;
	int cut_top, cur_cut_top;
	int cut_left, cur_cut_left;
	int cut_right, cur_cut_right;
	int cut_bot, cur_cut_bot;
	bool inverted;
	bool swapRedBlue;
	bool lockX;

	uint32_t width;
	uint32_t height;

	Pixmap pixmap;
	GLXPixmap glxpixmap;
	gs_texture_t tex;
	gs_texture_t gltex;

	pthread_mutex_t lock;
	pthread_mutexattr_t lockattr;
};


XCompcapMain::XCompcapMain(obs_data_t settings, obs_source_t source)
{
	p = new XCompcapMain_private;
	p->source = source;

	updateSettings(settings);
}

static void xcc_cleanup(XCompcapMain_private *p);

XCompcapMain::~XCompcapMain()
{
	ObsGsContextHolder obsctx;

	if (p->tex) {
		gs_texture_destroy(p->tex);
		p->tex = 0;
	}

	xcc_cleanup(p);

	delete p;
}

static Window getWindowFromString(std::string wstr)
{
	if (wstr == "") {
		return XCompcap::getTopLevelWindows().front();
	}

	if (wstr.substr(0, 4) == "root") {
		int i = std::stoi("0" + wstr.substr(4));
		return RootWindow(xdisp, i);
	}

	size_t firstMark = wstr.find(WIN_STRING_DIV);

	if (firstMark == std::string::npos)
		return (Window)std::stol(wstr);

	std::string widstr = wstr.substr(0, firstMark);
	Window wid = (Window)std::stol(widstr);

	wstr = wstr.substr(firstMark + strlen(WIN_STRING_DIV));

	size_t lastMark = wstr.rfind(WIN_STRING_DIV);
	std::string wname = wstr.substr(0, lastMark);

	Window matchedNameWin = wid;
	for (Window cwin: XCompcap::getTopLevelWindows()) {
		std::string cwinname = XCompcap::getWindowName(cwin);

		if (cwin == wid && wname == cwinname)
			return wid;

		if (wname == cwinname)
			matchedNameWin = cwin;
	}

	return matchedNameWin;
}

static void xcc_cleanup(XCompcapMain_private *p)
{
	if (p->gltex) {
		gs_texture_destroy(p->gltex);
		p->gltex = 0;
	}

	if (p->glxpixmap) {
		glXDestroyPixmap(xdisp, p->glxpixmap);
		p->glxpixmap = 0;
	}

	if (p->pixmap) {
		XFreePixmap(xdisp, p->pixmap);
		p->pixmap = 0;
	}

	if (p->win) {
		XCompositeUnredirectWindow(xdisp, p->win,
				CompositeRedirectAutomatic);
		XSelectInput(xdisp, p->win, 0);
		p->win = 0;
	}
}

void XCompcapMain::updateSettings(obs_data_t settings)
{
	PLock lock(&p->lock);
	XErrorLock xlock;
	ObsGsContextHolder obsctx;

	blog(LOG_DEBUG, "Settings updating");

	Window prevWin = p->win;

	xcc_cleanup(p);

	if (settings) {
		const char *windowName = obs_data_get_string(settings,
				"capture_window");

		p->win = getWindowFromString(windowName);

		p->cut_top = obs_data_get_int(settings, "cut_top");
		p->cut_left = obs_data_get_int(settings, "cut_left");
		p->cut_right = obs_data_get_int(settings, "cut_right");
		p->cut_bot = obs_data_get_int(settings, "cut_bot");
		p->lockX = obs_data_get_bool(settings, "lock_x");
		p->swapRedBlue = obs_data_get_bool(settings, "swap_redblue");
	} else {
		p->win = prevWin;
	}

	xlock.resetError();

	XCompositeRedirectWindow(xdisp, p->win, CompositeRedirectAutomatic);

	if (xlock.gotError()) {
		blog(LOG_ERROR, "XCompositeRedirectWindow failed: %s",
				xlock.getErrorText().c_str());
		return;
	}

	XSelectInput(xdisp, p->win, StructureNotifyMask);
	XSync(xdisp, 0);

	XWindowAttributes attr;
	if (!XGetWindowAttributes(xdisp, p->win, &attr)) {
		p->win = 0;
		p->width = 0;
		p->height = 0;
		return;
	}

	gs_color_format cf = GS_RGBA;

	p->width = attr.width;
	p->height = attr.height;

	if (p->cut_top + p->cut_bot < (int)p->height) {
		p->cur_cut_top = p->cut_top;
		p->cur_cut_bot = p->cut_bot;
	} else {
		p->cur_cut_top = 0;
		p->cur_cut_bot = 0;
	}

	if (p->cut_left + p->cut_right < (int)p->width) {
		p->cur_cut_left = p->cut_left;
		p->cur_cut_right = p->cut_right;
	} else {
		p->cur_cut_left = 0;
		p->cur_cut_right = 0;
	}

	if (p->tex)
		gs_texture_destroy(p->tex);

	uint8_t *texData = new uint8_t[width() * height() * 4];

	for (unsigned int i = 0; i < width() * height() * 4; i += 4) {
		texData[i + 0] = p->swapRedBlue ? 0 : 0xFF;
		texData[i + 1] = 0;
		texData[i + 2] = p->swapRedBlue ? 0xFF : 0;
		texData[i + 3] = 0xFF;
	}

	const uint8_t* texDataArr[] = { texData, 0 };

	p->tex = gs_texture_create(width(), height(), cf, 1,
			texDataArr, 0);

	delete[] texData;

	if (p->swapRedBlue) {
		GLuint tex = *(GLuint*)gs_texture_get_obj(p->tex);
		glBindTexture(GL_TEXTURE_2D, tex);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, GL_RED);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, GL_BLUE);
		glBindTexture(GL_TEXTURE_2D, 0);
	}

	const int attrs[] =
	{
		GLX_BIND_TO_TEXTURE_RGBA_EXT, GL_TRUE,
		GLX_DRAWABLE_TYPE, GLX_PIXMAP_BIT,
		GLX_BIND_TO_TEXTURE_TARGETS_EXT, GLX_TEXTURE_2D_BIT_EXT,
		GLX_DOUBLEBUFFER, GL_FALSE,
		None
	};

	int nelem = 0;
	GLXFBConfig* configs = glXChooseFBConfig(xdisp,
			XCompcap::getRootWindowScreen(attr.root),
			attrs, &nelem);

	if (nelem <= 0) {
		blog(LOG_ERROR, "no matching fb config found");
		p->win = 0;
		p->height = 0;
		p->width = 0;
		return;
	}

	glXGetFBConfigAttrib(xdisp, configs[0], GLX_Y_INVERTED_EXT, &nelem);
	p->inverted = nelem != 0;

	xlock.resetError();

	p->pixmap = XCompositeNameWindowPixmap(xdisp, p->win);

	if (xlock.gotError()) {
		blog(LOG_ERROR, "XCompositeNameWindowPixmap failed: %s",
				xlock.getErrorText().c_str());
		p->pixmap = 0;
		XFree(configs);
		return;
	}

	const int attribs[] =
	{
		GLX_TEXTURE_TARGET_EXT, GLX_TEXTURE_2D_EXT,
		GLX_TEXTURE_FORMAT_EXT, GLX_TEXTURE_FORMAT_RGBA_EXT,
		None
	};

	p->glxpixmap = glXCreatePixmap(xdisp, configs[0], p->pixmap, attribs);

	if (xlock.gotError()) {
		blog(LOG_ERROR, "glXCreatePixmap failed: %s",
				xlock.getErrorText().c_str());
		XFreePixmap(xdisp, p->pixmap);
		XFree(configs);
		p->pixmap = 0;
		p->glxpixmap = 0;
		return;
	}

	XFree(configs);

	p->gltex = gs_texture_create(p->width, p->height, cf, 1, 0,
			GS_GL_DUMMYTEX);

	GLuint gltex = *(GLuint*)gs_texture_get_obj(p->gltex);
	glBindTexture(GL_TEXTURE_2D, gltex);
	glXBindTexImageEXT(xdisp, p->glxpixmap, GLX_FRONT_LEFT_EXT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

void XCompcapMain::tick(float seconds)
{
	UNUSED_PARAMETER(seconds);

	PLock lock(&p->lock, true);

	if (!lock.isLocked())
		return;

	XCompcap::processEvents();

	if (XCompcap::windowWasReconfigured(p->win))
		updateSettings(0);

	if (!p->tex || !p->gltex)
		return;

	obs_enter_graphics();

	if (p->lockX) {
		XLockDisplay(xdisp);
		XSync(xdisp, 0);
	}

	gs_copy_texture_region(p->tex, 0, 0, p->gltex, p->cur_cut_left,
			p->cur_cut_top, width(), height());

	if (p->lockX)
		XUnlockDisplay(xdisp);

	obs_leave_graphics();
}

void XCompcapMain::render(gs_effect_t effect)
{
	PLock lock(&p->lock, true);

	if (!lock.isLocked() || !p->tex)
		return;

	gs_eparam_t image = gs_effect_get_param_by_name(effect, "image");
	gs_effect_set_texture(image, p->tex);

	gs_enable_blending(false);
	gs_draw_sprite(p->tex, 0, 0, 0);

	gs_reset_blend_state();
}

uint32_t XCompcapMain::width()
{
	return p->width - p->cur_cut_left - p->cur_cut_right;
}

uint32_t XCompcapMain::height()
{
	return p->height - p->cur_cut_bot - p->cur_cut_top;
}
