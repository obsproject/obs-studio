#include "OBSProjector.hpp"

#include <OBSApp.hpp>
#include <components/Multiview.hpp>
#include <utility/display-helpers.hpp>
#include <utility/platform.hpp>
#include <widgets/OBSBasic.hpp>

#include <qt-wrappers.hpp>

#include <QScreen>
#include <QWindow>

#include "moc_OBSProjector.cpp"
#include "OBSProjectorCustomSizeSetting.hpp"
#include <QRect>
#include <QScreen>

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

	OBSBasic *main = OBSBasic::Get();
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

			QMenu *resizeWindowMenu = this->GetWindowResizeMenu();
			popup.addMenu(resizeWindowMenu);
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
	OBSBasic *main = OBSBasic::Get();
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

QSize OBSProjector::GetTargetSize()
{
	OBSSource source = GetSource();
	if (source) {
		return QSize(std::max(obs_source_get_width(source), 1u), std::max(obs_source_get_height(source), 1u));
	} else {
		struct obs_video_info ovi;
		obs_get_video_info(&ovi);
		return QSize(ovi.base_width, ovi.base_height);
	}
}

void OBSProjector::ResizeToScale(int scale)
{
	std::pair<int, int> scaledSize = GetScaledSize(scale);
	resize(scaledSize.first, scaledSize.second);
}

void OBSProjector::ResizeToResolution(int width, int height)
{
	resize(width, height);
}

std::pair<int, int> OBSProjector::GetScaledSize(int scale)
{
	QSize targetSize = GetTargetSize();
	double scaleFactor = scale / 100.0;
	return std::make_pair(targetSize.width() * scaleFactor, targetSize.height() * scaleFactor);
}

QRect OBSProjector::GetScreenSize()
{
	QScreen *screen = this->window()->windowHandle()->screen();
	return screen->geometry();
}

std::vector<std::pair<int, int>> OBSProjector::GetResizeResolutionPresets()
{
	int resolutionPresets[][2] = {{1280, 720}, {1920, 1080}, {2560, 1440}, {3840, 2160}};
	std::vector<std::pair<int, int>> availablePresets;

	QRect screenSize = GetScreenSize();

	for (size_t i = 0; i < sizeof(resolutionPresets) / (sizeof(int) * 2); i++) {
		int width = resolutionPresets[i][0];
		int height = resolutionPresets[i][1];
		// don't include presets that are larger than screen size
		if (width < screenSize.width() && height < screenSize.height()) {
			availablePresets.push_back(std::make_pair(width, height));
		}
	}
	return availablePresets;
}

std::vector<int> OBSProjector::GetResizeScalePresets()
{
	int scalePresets[] = {50, 75, 100, 125, 150, 200};
	std::vector<int> availablePresets;

	QRect screenSize = GetScreenSize();

	for (size_t i = 0; i < sizeof(scalePresets) / sizeof(int); i++) {
		int scale = scalePresets[i];
		std::pair<int, int> scaledSize = GetScaledSize(scale);
		int width = scaledSize.first;
		int height = scaledSize.second;
		// Don't include presets that scales to larger than screen size,
		// except 100% which is always available
		if (scale == 100 || (width < screenSize.width() && height < screenSize.height())) {
			availablePresets.push_back(scale);
		}
	}
	return availablePresets;
}

QMenu *OBSProjector::GetWindowResizeMenu()
{
	// Resize Window menu
	QMenu *resizeWindowMenu = new QMenu(QTStr("ResizeProjectorWindow"));

	// Resize Window menu: preset resolution items
	auto resizeToResolutionAction = [this](QAction *action) {
		int width = action->property("width").toInt();
		int height = action->property("height").toInt();
		resize(width, height);
	};

	std::vector<std::pair<int, int>> resolutionPresets = GetResizeResolutionPresets();
	for (size_t i = 0; i < resolutionPresets.size(); i++) {
		int width = resolutionPresets[i].first;
		int height = resolutionPresets[i].second;
		QAction *resolution = new QAction(QString("%1x%2").arg(width).arg(height), this);
		resolution->setProperty("width", width);
		resolution->setProperty("height", height);
		connect(resolution, &QAction::triggered, std::bind(resizeToResolutionAction, resolution));
		resizeWindowMenu->addAction(resolution);
	}
	resizeWindowMenu->addSeparator();

	// Resize Window menu: preset scale items
	auto resizeToScaleAction = [this](QAction *action) {
		double scale = action->property("scale").toInt();
		ResizeToScale(scale);
	};

	std::vector<int> scalePresets = GetResizeScalePresets();

	for (size_t i = 0; i < scalePresets.size(); i++) {
		QAction *scale = new QAction(QString("%1%").arg(scalePresets[i]), this);
		scale->setProperty("scale", scalePresets[i]);
		connect(scale, &QAction::triggered, std::bind(resizeToScaleAction, scale));
		resizeWindowMenu->addAction(scale);
	}

	resizeWindowMenu->addSeparator();

	QAction *custom = new QAction(QTStr("ResizeProjectorWindowCustom"), this);
	connect(custom, &QAction::triggered, this, &OBSProjector::OpenCustomWindowSizeDialog);
	resizeWindowMenu->addAction(custom);

	return resizeWindowMenu;
}

void OBSProjector::OpenCustomWindowSizeDialog()
{
	OBSProjectorCustomSizeSetting *dialog = new OBSProjectorCustomSizeSetting(this);
	connect(dialog, &OBSProjectorCustomSizeSetting::ApplyResolution, this, &OBSProjector::ResizeToResolution);
	connect(dialog, &OBSProjectorCustomSizeSetting::ApplyScale, this, &OBSProjector::ResizeToScale);
	dialog->open();
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
