#include <QAction>
#include <QGuiApplication>
#include <QMouseEvent>
#include <QMenu>
#include <QScreen>
#include <qt-wrappers.hpp>
#include "moc_window-projector.cpp"
#include "obs-app.hpp"
#include "window-basic-main.hpp"
#include "display-helpers.hpp"
#include "platform.hpp"
#include "multiview.hpp"

static QList<OBSProjector *> multiviewProjectors;

static bool updatingMultiview = false, mouseSwitching, transitionOnDoubleClick;

OBSProjector::OBSProjector(QWidget *widget, obs_source_t *source_, int monitor, ProjectorType type_)
	: OBSQTDisplay(widget, Qt::Window),
	  weakSource(OBSGetWeakRef(source_))
{
	OBSSource source = GetSource();
	if (source) {
		sigs.emplace_back(obs_source_get_signal_handler(source), "rename", OBSSourceRenamed, this);
		sigs.emplace_back(obs_source_get_signal_handler(source), "destroy", OBSSourceDestroyed, this);
	}

	isAlwaysOnTop = config_get_bool(App()->GetUserConfig(), "BasicWindow", "ProjectorAlwaysOnTop");

	if (isAlwaysOnTop)
		setWindowFlags(Qt::WindowStaysOnTopHint);

	// Mark the window as a projector so SetDisplayAffinity
	// can skip it
	windowHandle()->setProperty("isOBSProjectorWindow", true);

#if defined(__linux__) || defined(__FreeBSD__) || defined(__DragonFly__)
	// Prevents resizing of projector windows
	setAttribute(Qt::WA_PaintOnScreen, false);
#endif

	type = type_;
#ifdef __APPLE__
	setWindowIcon(QIcon::fromTheme("obs", QIcon(":/res/images/obs_256x256.png")));
#else
	setWindowIcon(QIcon::fromTheme("obs", QIcon(":/res/images/obs.png")));
#endif

	if (monitor == -1)
		resize(480, 270);
	else
		SetMonitor(monitor);

	if (source)
		UpdateProjectorTitle(QT_UTF8(obs_source_get_name(source)));
	else
		UpdateProjectorTitle(QString());

	QAction *action = new QAction(this);
	action->setShortcut(Qt::Key_Escape);
	addAction(action);
	connect(action, &QAction::triggered, this, &OBSProjector::EscapeTriggered);

	setAttribute(Qt::WA_DeleteOnClose, true);

	//disable application quit when last window closed
	setAttribute(Qt::WA_QuitOnClose, false);

	installEventFilter(CreateShortcutFilter());

	auto addDrawCallback = [this]() {
		bool isMultiview = type == ProjectorType::Multiview;
		obs_display_add_draw_callback(GetDisplay(), isMultiview ? OBSRenderMultiview : OBSRender, this);
		obs_display_set_background_color(GetDisplay(), 0x000000);
	};

	connect(this, &OBSQTDisplay::DisplayCreated, addDrawCallback);
	connect(App(), &QGuiApplication::screenRemoved, this, &OBSProjector::ScreenRemoved);

	if (type == ProjectorType::Multiview) {
		multiview = new Multiview();

		UpdateMultiview();

		multiviewProjectors.push_back(this);
	}

	App()->IncrementSleepInhibition();

	if (source)
		obs_source_inc_showing(source);

	ready = true;

	show();

	// We need it here to allow keyboard input in X11 to listen to Escape
	activateWindow();
}

OBSProjector::~OBSProjector()
{
	sigs.clear();

	bool isMultiview = type == ProjectorType::Multiview;
	obs_display_remove_draw_callback(GetDisplay(), isMultiview ? OBSRenderMultiview : OBSRender, this);

	OBSSource source = GetSource();
	if (source)
		obs_source_dec_showing(source);

	if (isMultiview) {
		delete multiview;
		multiviewProjectors.removeAll(this);
	}

	App()->DecrementSleepInhibition();
}

void OBSProjector::SetMonitor(int monitor)
{
	savedMonitor = monitor;
	setGeometry(QGuiApplication::screens()[monitor]->geometry());
	showFullScreen();
	SetHideCursor();
}

void OBSProjector::SetHideCursor()
{
	if (savedMonitor == -1)
		return;

	bool hideCursor = config_get_bool(App()->GetUserConfig(), "BasicWindow", "HideProjectorCursor");

	if (hideCursor && type != ProjectorType::Multiview)
		setCursor(Qt::BlankCursor);
	else
		setCursor(Qt::ArrowCursor);
}

void OBSProjector::OBSRenderMultiview(void *data, uint32_t cx, uint32_t cy)
{
	OBSProjector *window = (OBSProjector *)data;

	if (updatingMultiview || !window->ready)
		return;

	window->multiview->Render(cx, cy);
}

void OBSProjector::OBSRender(void *data, uint32_t cx, uint32_t cy)
{
	OBSProjector *window = reinterpret_cast<OBSProjector *>(data);

	if (!window->ready)
		return;

	OBSBasic *main = reinterpret_cast<OBSBasic *>(App()->GetMainWindow());
	OBSSource source = window->GetSource();

	uint32_t targetCX;
	uint32_t targetCY;
	int x, y;
	int newCX, newCY;
	float scale;

	if (source) {
		targetCX = std::max(obs_source_get_width(source), 1u);
		targetCY = std::max(obs_source_get_height(source), 1u);
	} else {
		struct obs_video_info ovi;
		obs_get_video_info(&ovi);
		targetCX = ovi.base_width;
		targetCY = ovi.base_height;
	}

	GetScaleAndCenterPos(targetCX, targetCY, cx, cy, x, y, scale);

	newCX = int(scale * float(targetCX));
	newCY = int(scale * float(targetCY));

	startRegion(x, y, newCX, newCY, 0.0f, float(targetCX), 0.0f, float(targetCY));

	if (window->type == ProjectorType::Preview && main->IsPreviewProgramMode()) {
		OBSSource curSource = main->GetCurrentSceneSource();

		if (source != curSource) {
			obs_source_dec_showing(source);
			obs_source_inc_showing(curSource);
			source = curSource;
			window->weakSource = OBSGetWeakRef(source);
		}
	} else if (window->type == ProjectorType::Preview && !main->IsPreviewProgramMode()) {
		window->weakSource = nullptr;
	}

	if (source)
		obs_source_video_render(source);
	else
		obs_render_main_texture();

	endRegion();
}

void OBSProjector::OBSSourceRenamed(void *data, calldata_t *params)
{
	OBSProjector *window = reinterpret_cast<OBSProjector *>(data);
	QString oldName = calldata_string(params, "prev_name");
	QString newName = calldata_string(params, "new_name");

	QMetaObject::invokeMethod(window, "RenameProjector", Q_ARG(QString, oldName), Q_ARG(QString, newName));
}

void OBSProjector::OBSSourceDestroyed(void *data, calldata_t *)
{
	OBSProjector *window = reinterpret_cast<OBSProjector *>(data);
	QMetaObject::invokeMethod(window, "EscapeTriggered");
}

void OBSProjector::mouseDoubleClickEvent(QMouseEvent *event)
{
	OBSQTDisplay::mouseDoubleClickEvent(event);

	if (!mouseSwitching)
		return;

	if (!transitionOnDoubleClick)
		return;

	// Only MultiView projectors handle double click
	if (this->type != ProjectorType::Multiview)
		return;

	OBSBasic *main = (OBSBasic *)obs_frontend_get_main_window();
	if (!main->IsPreviewProgramMode())
		return;

	if (event->button() == Qt::LeftButton) {
		QPoint pos = event->pos();
		OBSSource src = multiview->GetSourceByPosition(pos.x(), pos.y());
		if (!src)
			return;

		if (main->GetProgramSource() != src)
			main->TransitionToScene(src);
	}
}

void OBSProjector::mousePressEvent(QMouseEvent *event)
{
	OBSQTDisplay::mousePressEvent(event);

	if (event->button() == Qt::RightButton) {
		QMenu *projectorMenu = new QMenu(QTStr("Fullscreen"));
		OBSBasic::AddProjectorMenuMonitors(projectorMenu, this, &OBSProjector::OpenFullScreenProjector);

		QMenu popup(this);
		popup.addMenu(projectorMenu);

		if (GetMonitor() > -1) {
			popup.addAction(QTStr("Windowed"), this, &OBSProjector::OpenWindowedProjector);

		} else if (!this->isMaximized()) {
			popup.addAction(QTStr("ResizeProjectorWindowToContent"), this, &OBSProjector::ResizeToContent);
		}

		QAction *alwaysOnTopButton = new QAction(QTStr("Basic.MainMenu.View.AlwaysOnTop"), this);
		alwaysOnTopButton->setCheckable(true);
		alwaysOnTopButton->setChecked(isAlwaysOnTop);

		connect(alwaysOnTopButton, &QAction::toggled, this, &OBSProjector::AlwaysOnTopToggled);

		popup.addAction(alwaysOnTopButton);

		popup.addAction(QTStr("Close"), this, &OBSProjector::EscapeTriggered);
		popup.exec(QCursor::pos());
	} else if (event->button() == Qt::LeftButton) {
		// Only MultiView projectors handle left click
		if (this->type != ProjectorType::Multiview)
			return;

		if (!mouseSwitching)
			return;

		QPoint pos = event->pos();
		OBSSource src = multiview->GetSourceByPosition(pos.x(), pos.y());
		if (!src)
			return;

		OBSBasic *main = (OBSBasic *)obs_frontend_get_main_window();
		if (main->GetCurrentSceneSource() != src)
			main->SetCurrentScene(src, false);
	}
}

void OBSProjector::EscapeTriggered()
{
	OBSBasic *main = reinterpret_cast<OBSBasic *>(App()->GetMainWindow());
	main->DeleteProjector(this);
}

void OBSProjector::UpdateMultiview()
{
	MultiviewLayout multiviewLayout =
		static_cast<MultiviewLayout>(config_get_int(App()->GetUserConfig(), "BasicWindow", "MultiviewLayout"));

	bool drawLabel = config_get_bool(App()->GetUserConfig(), "BasicWindow", "MultiviewDrawNames");

	bool drawSafeArea = config_get_bool(App()->GetUserConfig(), "BasicWindow", "MultiviewDrawAreas");

	mouseSwitching = config_get_bool(App()->GetUserConfig(), "BasicWindow", "MultiviewMouseSwitch");

	transitionOnDoubleClick = config_get_bool(App()->GetUserConfig(), "BasicWindow", "TransitionOnDoubleClick");

	multiview->Update(multiviewLayout, drawLabel, drawSafeArea);
}

void OBSProjector::UpdateProjectorTitle(QString name)
{
	bool window = (GetMonitor() == -1);

	QString title = nullptr;
	switch (type) {
	case ProjectorType::Scene:
		if (!window)
			title = QTStr("SceneProjector") + " - " + name;
		else
			title = QTStr("SceneWindow") + " - " + name;
		break;
	case ProjectorType::Source:
		if (!window)
			title = QTStr("SourceProjector") + " - " + name;
		else
			title = QTStr("SourceWindow") + " - " + name;
		break;
	case ProjectorType::Preview:
		if (!window)
			title = QTStr("PreviewProjector");
		else
			title = QTStr("PreviewWindow");
		break;
	case ProjectorType::StudioProgram:
		if (!window)
			title = QTStr("StudioProgramProjector");
		else
			title = QTStr("StudioProgramWindow");
		break;
	case ProjectorType::Multiview:
		if (!window)
			title = QTStr("MultiviewProjector");
		else
			title = QTStr("MultiviewWindowed");
		break;
	default:
		title = name;
		break;
	}

	setWindowTitle(title);
}

OBSSource OBSProjector::GetSource()
{
	return OBSGetStrongRef(weakSource);
}

ProjectorType OBSProjector::GetProjectorType()
{
	return type;
}

int OBSProjector::GetMonitor()
{
	return savedMonitor;
}

void OBSProjector::UpdateMultiviewProjectors()
{
	obs_enter_graphics();
	updatingMultiview = true;
	obs_leave_graphics();

	for (auto &projector : multiviewProjectors)
		projector->UpdateMultiview();

	obs_enter_graphics();
	updatingMultiview = false;
	obs_leave_graphics();
}

void OBSProjector::RenameProjector(QString oldName, QString newName)
{
	if (oldName == newName)
		return;

	UpdateProjectorTitle(newName);
}

void OBSProjector::OpenFullScreenProjector()
{
	if (!isFullScreen())
		prevGeometry = geometry();

	int monitor = sender()->property("monitor").toInt();
	SetMonitor(monitor);

	OBSSource source = GetSource();
	UpdateProjectorTitle(QT_UTF8(obs_source_get_name(source)));
}

void OBSProjector::OpenWindowedProjector()
{
	showFullScreen();
	showNormal();
	setCursor(Qt::ArrowCursor);

	if (!prevGeometry.isNull())
		setGeometry(prevGeometry);
	else
		resize(480, 270);

	savedMonitor = -1;

	OBSSource source = GetSource();
	UpdateProjectorTitle(QT_UTF8(obs_source_get_name(source)));
}

void OBSProjector::ResizeToContent()
{
	OBSSource source = GetSource();
	uint32_t targetCX;
	uint32_t targetCY;
	int x, y, newX, newY;
	float scale;

	if (source) {
		targetCX = std::max(obs_source_get_width(source), 1u);
		targetCY = std::max(obs_source_get_height(source), 1u);
	} else {
		struct obs_video_info ovi;
		obs_get_video_info(&ovi);
		targetCX = ovi.base_width;
		targetCY = ovi.base_height;
	}

	QSize size = this->size();
	GetScaleAndCenterPos(targetCX, targetCY, size.width(), size.height(), x, y, scale);

	newX = size.width() - (x * 2);
	newY = size.height() - (y * 2);
	resize(newX, newY);
}

void OBSProjector::AlwaysOnTopToggled(bool isAlwaysOnTop)
{
	SetIsAlwaysOnTop(isAlwaysOnTop, true);
}

void OBSProjector::closeEvent(QCloseEvent *event)
{
	EscapeTriggered();
	event->accept();
}

bool OBSProjector::IsAlwaysOnTop() const
{
	return isAlwaysOnTop;
}

bool OBSProjector::IsAlwaysOnTopOverridden() const
{
	return isAlwaysOnTopOverridden;
}

void OBSProjector::SetIsAlwaysOnTop(bool isAlwaysOnTop, bool isOverridden)
{
	this->isAlwaysOnTop = isAlwaysOnTop;
	this->isAlwaysOnTopOverridden = isOverridden;

	SetAlwaysOnTop(this, isAlwaysOnTop);
}

void OBSProjector::ScreenRemoved(QScreen *screen)
{
	if (GetMonitor() < 0)
		return;

	if (screen == this->screen())
		EscapeTriggered();
}
