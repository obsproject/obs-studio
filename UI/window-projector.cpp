#include <QAction>
#include <QGuiApplication>
#include <QMouseEvent>
#include <QMenu>
#include <QScreen>
#include "window-projector.hpp"
#include "display-helpers.hpp"
#include "qt-wrappers.hpp"
#include "platform.hpp"

OBSProjector::OBSProjector(QWidget *widget, obs_source_t *source_, bool window)
	: OBSQTDisplay                 (widget,
	                                Qt::Window),
	  source                       (source_),
	  removedSignal                (obs_source_get_signal_handler(source),
	                                "remove", OBSSourceRemoved, this)
{
	if (!window) {
		setWindowFlags(Qt::FramelessWindowHint |
				Qt::X11BypassWindowManagerHint);
	}

	setAttribute(Qt::WA_DeleteOnClose, true);

	//disable application quit when last window closed
	setAttribute(Qt::WA_QuitOnClose, false);

	installEventFilter(CreateShortcutFilter());

	auto addDrawCallback = [this] ()
	{
		obs_display_add_draw_callback(GetDisplay(),
				isMultiview ? OBSRenderMultiview : OBSRender,
				this);
		obs_display_set_background_color(GetDisplay(), 0x000000);
	};

	connect(this, &OBSQTDisplay::DisplayCreated, addDrawCallback);

	bool hideCursor = config_get_bool(GetGlobalConfig(),
			"BasicWindow", "HideProjectorCursor");
	if (hideCursor && !window) {
		QPixmap empty(16, 16);
		empty.fill(Qt::transparent);
		setCursor(QCursor(empty));
	}

	App()->IncrementSleepInhibition();
	resize(480, 270);
}

OBSProjector::~OBSProjector()
{
	obs_display_remove_draw_callback(GetDisplay(),
			isMultiview ? OBSRenderMultiview : OBSRender, this);

	if (source)
		obs_source_dec_showing(source);

	if (isMultiview) {
		for (size_t i = 0; i < multiviewScenes.size(); i++) {
			obs_source_t *src = multiviewScenes.at(i);
			if (src)
				obs_source_dec_showing(src);
		}
		for (size_t i = 0; i < multiviewLabels.size(); i++) {
			obs_source_release(multiviewLabels.at(i++));

			obs_source_t *lbl = multiviewLabels.at(i);
			if (lbl)
				obs_source_dec_showing(lbl);
			obs_source_release(lbl);
		}

		obs_enter_graphics();
		gs_vertexbuffer_destroy(outerBox);
		gs_vertexbuffer_destroy(innerBox);
		gs_vertexbuffer_destroy(leftVLine);
		gs_vertexbuffer_destroy(rightVLine);
		gs_vertexbuffer_destroy(leftLine);
		gs_vertexbuffer_destroy(topLine);
		gs_vertexbuffer_destroy(rightLine);
		obs_leave_graphics();
	}

	App()->DecrementSleepInhibition();
}

obs_source_t *createTextBackgroundSource(size_t len, size_t w, size_t h)
{
	obs_data_t *settings = obs_data_create();

	obs_data_set_int(settings, "color", 3642695455);
	obs_data_set_int(settings, "height", int(h * 0.14));
	obs_data_set_int(settings, "width", int(w * 0.038 * len));

	obs_source_t *txtSource = obs_source_create("color_source",
			"background", settings, nullptr);

	obs_data_release(settings);

	return txtSource;
}

obs_source_t *createTextSource(const char *name, size_t h)
{
	obs_data_t *settings = obs_data_create();
	obs_data_t *font     = obs_data_create();

#if defined(_WIN32)
	obs_data_set_string(font, "face", "Arial");
#elif defined(__APPLE__)
	obs_data_set_string(font, "face", "Helvetica");
#else
	obs_data_set_string(font, "face", "Monospace");
#endif
	obs_data_set_int(font, "flags", 0);
	obs_data_set_int(font, "size", int(h / 9.81));

	obs_data_set_obj(settings, "font", font);
	obs_data_set_string(settings, "text", name);
	obs_data_set_bool(settings, "outline", true);

	obs_source_t *txtSource = obs_source_create("text_ft2_source", name,
			settings, nullptr);

	obs_data_release(font);
	obs_data_release(settings);

	return txtSource;
}

void OBSProjector::Init(int monitor, bool window, QString title, bool multiView)
{
	QScreen *screen = QGuiApplication::screens()[monitor];

	if (!window)
		setGeometry(screen->geometry());

	bool alwaysOnTop = config_get_bool(GetGlobalConfig(),
			"BasicWindow", "ProjectorAlwaysOnTop");
	if (alwaysOnTop && !window)
		SetAlwaysOnTop(this, true);

	if (window)
		setWindowTitle(title);

	show();

	if (source)
		obs_source_inc_showing(source);

	if (!window) {
		QAction *action = new QAction(this);
		action->setShortcut(Qt::Key_Escape);
		addAction(action);
		connect(action, SIGNAL(triggered()), this,
				SLOT(EscapeTriggered()));
		activateWindow();
	}

	savedMonitor = monitor;
	isWindow     = window;
	isMultiview  = multiView;

	if (isMultiview) {
		struct obs_video_info ovi;
		obs_get_video_info(&ovi);
		size_t w = ovi.base_width;
		size_t h = ovi.base_height;

		multiviewLabels.emplace_back(
				createTextBackgroundSource(7, w, h));
		multiviewLabels.emplace_back(createTextSource("Preview", h));
		multiviewLabels.emplace_back(
				createTextBackgroundSource(7, w, h));
		multiviewLabels.emplace_back(createTextSource("Program", h));

		struct obs_frontend_source_list scenes = {};
		obs_frontend_get_scenes(&scenes);
		for (size_t i = 0; i < scenes.sources.num && i < 8; i++) {
			obs_source_t *src = scenes.sources.array[i];
			multiviewScenes.emplace_back(src);
			obs_source_inc_showing(src);

			std::string tmpStr = QString("%1 %2")
					.arg(QString::number(i + 1),
							obs_source_get_name(
									src))
					.toStdString();
			const char  *name = tmpStr.size() > 15 ? tmpStr
					.substr(0, 15).c_str() : tmpStr.c_str();
			multiviewLabels.emplace_back(
					createTextBackgroundSource(strlen(name),
							w, h));
			multiviewLabels.emplace_back(createTextSource(name, h));
		}
		obs_frontend_source_list_free(&scenes);

		obs_enter_graphics();
		gs_render_start(true);
		gs_vertex2f(0.001f, 0.001f);
		gs_vertex2f(0.001f, 0.997f);
		gs_vertex2f(0.997f, 0.997f);
		gs_vertex2f(0.997f, 0.001f);
		gs_vertex2f(0.001f, 0.001f);
		outerBox = gs_render_save();

		gs_render_start(true);
		gs_vertex2f(0.04f, 0.04f);
		gs_vertex2f(0.04f, 0.96f);
		gs_vertex2f(0.96f, 0.96f);
		gs_vertex2f(0.96f, 0.04f);
		gs_vertex2f(0.04f, 0.04f);
		innerBox = gs_render_save();

		gs_render_start(true);
		gs_vertex2f(0.15f, 0.04f);
		gs_vertex2f(0.15f, 0.96f);
		leftVLine = gs_render_save();

		gs_render_start(true);
		gs_vertex2f(0.85f, 0.04f);
		gs_vertex2f(0.85f, 0.96f);
		rightVLine = gs_render_save();

		gs_render_start(true);
		gs_vertex2f(0.0f, 0.5f);
		gs_vertex2f(0.075f, 0.5f);
		leftLine = gs_render_save();

		gs_render_start(true);
		gs_vertex2f(0.5f, 0.0f);
		gs_vertex2f(0.5f, 0.09f);
		topLine = gs_render_save();

		gs_render_start(true);
		gs_vertex2f(0.925f, 0.5f);
		gs_vertex2f(1.0f, 0.5f);
		rightLine = gs_render_save();
		obs_leave_graphics();
	}
}

static inline void renderBox(gs_vertbuffer_t *vb, int cx, int cy,
		source_type type)
{
	if (!vb)
		return;

	gs_effect_t    *solid = obs_get_base_effect(OBS_EFFECT_SOLID);
	gs_technique_t *tech  = gs_effect_get_technique(solid, "Solid");
	vec4           color;

	switch (type) {
	case PROGRAM:
		vec4_set(&color, 1.0f, 0.0f, 0.0f, 1.0f);
		break;
	case PREVIEW:
		vec4_set(&color, 0.0f, 1.0f, 0.0f, 1.0f);
		break;
	case SOURCE:
		vec4_set(&color, 1.0f, 1.0f, 1.0f, 1.0f);
		break;
	}

	gs_effect_set_vec4(gs_effect_get_param_by_name(solid, "color"), &color);

	matrix4 transform;
	matrix4_identity(&transform);
	transform.x.x = cx;
	transform.y.y = cy;

	gs_technique_begin(tech);
	gs_technique_begin_pass(tech, 0);
	gs_matrix_push();
	gs_matrix_mul(&transform);

	gs_load_vertexbuffer(vb);
	gs_draw(GS_LINESTRIP, 0, 0);

	gs_matrix_pop();
	gs_technique_end_pass(tech);
	gs_technique_end(tech);
}

static inline size_t variableLen(int x, size_t len)
{
	return (x / 2) - (1 + (x / 27) * (len / 2));
}

void OBSProjector::OBSRenderMultiview(void *data, uint32_t cx, uint32_t cy)
{
	OBSProjector *window = reinterpret_cast<OBSProjector *>(data);
	OBSBasic     *main   = (OBSBasic *) obs_frontend_get_main_window();
	uint32_t     targetCX, targetCY;
	int          x, y, newCX, newCY, halfCX, halfCY, sourceX, sourceY,
		     quarterCX, quarterCY;
	float        scale, targetCXF, targetCYF;

	struct obs_video_info ovi;
	obs_get_video_info(&ovi);
	targetCX = ovi.base_width;
	targetCY = ovi.base_height;

	GetScaleAndCenterPos(targetCX, targetCY, cx, cy, x, y, scale);

	targetCXF = float(targetCX);
	targetCYF = float(targetCY);
	newCX     = int(scale * targetCXF);
	newCY     = int(scale * targetCYF);
	halfCX    = (newCX + 1) / 2;
	halfCY    = (newCY + 1) / 2;
	quarterCX = (halfCX + 1) / 2;
	quarterCY = (halfCY + 1) / 2;

	const char   *previewName = obs_source_get_name(
			main->GetCurrentSceneSource());
	const char   *programName = obs_source_get_name(
			main->GetProgramSource());
	obs_source_t *previewSrc  = window->multiviewLabels.at(1);
	obs_source_t *programSrc  = window->multiviewLabels.at(3);

	gs_viewport_push();
	gs_projection_push();
	gs_ortho(0.0f, targetCXF, 0.0f, targetCYF, -100.0f, 100.0f);

	for (size_t i = 0; i < window->multiviewScenes.size() && i < 8; i++) {
		obs_source_t *src = window->multiviewScenes.at(i);
		obs_source_t *lbl = window->multiviewLabels.at(2 * i + 5);
		if (!src)
			continue;

		const char *name      = obs_source_get_name(src);
		const char *labelName = obs_source_get_name(lbl);
		size_t     labelLen   = strlen(labelName);
		size_t     labelVar   = variableLen(quarterCX, labelLen);

		if (i < 4) {
			sourceX = x + (i * quarterCX);
			sourceY = y + halfCY;
		} else {
			sourceX = x + ((i - 4) * quarterCX);
			sourceY = y + halfCY + quarterCY;
		}

		gs_set_viewport(sourceX, sourceY, quarterCX, quarterCY);
		obs_source_video_render(src);
		renderBox(window->outerBox, targetCX, targetCY, SOURCE);

		if (strcmp(name, previewName) == 0) {
			gs_set_viewport(sourceX, sourceY, quarterCX, quarterCY);
			renderBox(window->outerBox, targetCX, targetCY,
					PREVIEW);

			gs_set_viewport(x, y, halfCX, halfCY);
			obs_source_video_render(src);
			renderBox(window->outerBox, targetCX, targetCY, SOURCE);
			renderBox(window->innerBox, targetCX, targetCY, SOURCE);
			renderBox(window->leftVLine, targetCX, targetCY,
					SOURCE);
			renderBox(window->rightVLine, targetCX, targetCY,
					SOURCE);
			renderBox(window->leftLine, targetCX, targetCY, SOURCE);
			renderBox(window->topLine, targetCX, targetCY, SOURCE);
			renderBox(window->rightLine, targetCX, targetCY,
					SOURCE);

			gs_set_viewport(x + variableLen(halfCX, 7),
					(halfCY / 1.25) + y, halfCX, halfCY);
			obs_source_video_render(window->multiviewLabels.at(0));
			obs_source_video_render(previewSrc);
		}

		if (strcmp(name, programName) == 0) {
			gs_set_viewport(x + halfCX, y, halfCX, halfCY);
			obs_source_video_render(src);
			renderBox(window->outerBox, targetCX, targetCY, SOURCE);

			gs_set_viewport(sourceX, sourceY, quarterCX, quarterCY);
			renderBox(window->outerBox, targetCX, targetCY,
					PROGRAM);

			gs_set_viewport(x + halfCX + variableLen(halfCX, 7),
					(halfCY / 1.25) + y, halfCX, halfCY);
			obs_source_video_render(window->multiviewLabels.at(0));
			obs_source_video_render(programSrc);
		}

		gs_set_viewport(sourceX + labelVar,
				(quarterCY / 1.25) + sourceY, quarterCX,
				quarterCY);
		obs_source_video_render(window->multiviewLabels.at(2 * i + 4));
		obs_source_video_render(lbl);
	}

	gs_projection_pop();
	gs_viewport_pop();
}

void OBSProjector::OBSRender(void *data, uint32_t cx, uint32_t cy)
{
	OBSProjector *window = reinterpret_cast<OBSProjector*>(data);

	uint32_t targetCX;
	uint32_t targetCY;
	int      x, y;
	int      newCX, newCY;
	float    scale;

	if (window->source) {
		targetCX = std::max(obs_source_get_width(window->source), 1u);
		targetCY = std::max(obs_source_get_height(window->source), 1u);
	} else {
		struct obs_video_info ovi;
		obs_get_video_info(&ovi);
		targetCX = ovi.base_width;
		targetCY = ovi.base_height;
	}

	GetScaleAndCenterPos(targetCX, targetCY, cx, cy, x, y, scale);

	newCX = int(scale * float(targetCX));
	newCY = int(scale * float(targetCY));

	gs_viewport_push();
	gs_projection_push();
	gs_ortho(0.0f, float(targetCX), 0.0f, float(targetCY), -100.0f, 100.0f);
	gs_set_viewport(x, y, newCX, newCY);

	if (window->source)
		obs_source_video_render(window->source);
	else
		obs_render_main_view();

	gs_projection_pop();
	gs_viewport_pop();
}

void OBSProjector::OBSSourceRemoved(void *data, calldata_t *params)
{
	OBSProjector *window = reinterpret_cast<OBSProjector*>(data);

	window->deleteLater();

	UNUSED_PARAMETER(params);
}

void OBSProjector::mousePressEvent(QMouseEvent *event)
{
	OBSQTDisplay::mousePressEvent(event);

	if (event->button() == Qt::RightButton) {
		QMenu popup(this);
		popup.addAction(QTStr("Close"), this, SLOT(EscapeTriggered()));
		popup.exec(QCursor::pos());
	}
}

void OBSProjector::EscapeTriggered()
{
	if (!isWindow) {
		OBSBasic *main = (OBSBasic*)obs_frontend_get_main_window();
		main->RemoveSavedProjectors(savedMonitor);
	}

	deleteLater();
}
