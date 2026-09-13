#include <utility/PreviewLayout.hpp>

#include <QApplication>
#include <QLabel>
#include <QMainWindow>
#include <QMenuBar>
#include <QStatusBar>
#include <QToolBar>
#include <QVBoxLayout>

#include <cmath>
#include <iostream>

static int failures = 0;

#define CHECK(cond)                                                                    \
	do {                                                                           \
		if (!(cond)) {                                                         \
			std::cerr << "FAIL " << __LINE__ << ": " #cond << std::endl;   \
			failures++;                                                    \
		}                                                                      \
	} while (0)

static bool near(double a, double b, double tolerance = 0.0001)
{
	return std::fabs(a - b) <= tolerance;
}

static void settle()
{
	QCoreApplication::processEvents();
	QCoreApplication::sendPostedEvents(nullptr, QEvent::LayoutRequest);
	QCoreApplication::processEvents();
}

static QDockWidget *makeDock(QMainWindow *window, const char *name, Qt::DockWidgetArea area, QSize minimum)
{
	auto *dock = new QDockWidget(name, window);
	dock->setObjectName(name);
	auto *content = new QLabel(name, dock);
	content->setMinimumSize(minimum);
	dock->setWidget(content);
	window->addDockWidget(area, dock);
	return dock;
}

/* A window shaped like the Nova layout: header toolbar, scenes/sources on the
 * left, broadcast controls on the right, mixer/transitions/tools below. */
struct StudioWindow {
	QMainWindow window;
	QDockWidget *scenes, *sources, *controls, *mixer, *transitions, *tools;

	StudioWindow()
	{
		window.setDockOptions(QMainWindow::AllowNestedDocks | QMainWindow::AllowTabbedDocks);
		window.setCorner(Qt::BottomLeftCorner, Qt::BottomDockWidgetArea);
		window.setCorner(Qt::BottomRightCorner, Qt::BottomDockWidgetArea);
		window.menuBar()->addMenu("View");
		auto *header = new QToolBar("Header", &window);
		header->setObjectName("novaHeader");
		auto *brand = new QLabel("nova", header);
		brand->setMinimumHeight(60);
		header->addWidget(brand);
		window.addToolBar(Qt::TopToolBarArea, header);
		window.statusBar()->showMessage("ready");

		auto *central = new QWidget(&window);
		auto *layout = new QVBoxLayout(central);
		auto *preview = new QLabel("preview", central);
		preview->setMinimumSize(32, 32);
		preview->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
		layout->addWidget(preview);
		window.setCentralWidget(central);

		scenes = makeDock(&window, "scenesDock", Qt::LeftDockWidgetArea, QSize(120, 80));
		sources = makeDock(&window, "sourcesDock", Qt::LeftDockWidgetArea, QSize(120, 80));
		window.splitDockWidget(scenes, sources, Qt::Vertical);
		controls = makeDock(&window, "controlsDock", Qt::RightDockWidgetArea, QSize(150, 200));
		mixer = makeDock(&window, "mixerDock", Qt::BottomDockWidgetArea, QSize(200, 60));
		transitions = makeDock(&window, "transitionsDock", Qt::BottomDockWidgetArea, QSize(120, 60));
		tools = makeDock(&window, "novaToolsDock", Qt::BottomDockWidgetArea, QSize(120, 110));
		window.splitDockWidget(mixer, transitions, Qt::Horizontal);
		window.splitDockWidget(transitions, tools, Qt::Horizontal);
	}
};

static void testMath()
{
	CHECK(near(PreviewLayout::Clamp(0.1), PreviewLayout::kMinShare));
	CHECK(near(PreviewLayout::Clamp(0.95), PreviewLayout::kMaxShare));
	CHECK(near(PreviewLayout::Clamp(0.5), 0.5));

	CHECK(near(PreviewLayout::Share(500, 1000), 0.5));
	CHECK(near(PreviewLayout::Share(0, 1000), 0.0));
	CHECK(near(PreviewLayout::Share(500, 0), 0.0));
	CHECK(near(PreviewLayout::Share(1500, 1000), 1.0));

	/* 1000px window, chrome takes 100px: central 300 + docks 600. Asking for
	 * 60% leaves 900 - 600 = 300 for docks. */
	CHECK(PreviewLayout::DockExtentForShare(0.6, 1000, 300, 600, 100) == 300);
	/* Docks never drop below their minimum. */
	CHECK(PreviewLayout::DockExtentForShare(0.9, 1000, 300, 600, 250) == 250);
	/* Docks grow when the preview has more than its share. */
	CHECK(PreviewLayout::DockExtentForShare(0.3, 1000, 900, 100, 0) == 700);
	/* When the window is too small for the share, docks fall to their minimum. */
	CHECK(PreviewLayout::DockExtentForShare(0.3, 1000, 100, 100, 0) == 0);
	/* Negative inputs are treated as empty. */
	CHECK(PreviewLayout::DockExtentForShare(0.5, 1000, -5, -5, -5) == 0);

	QList<int> split = PreviewLayout::Distribute(300, {200, 100}, {0, 0});
	CHECK(split.size() == 2 && split[0] == 200 && split[1] == 100);
	split = PreviewLayout::Distribute(150, {200, 100}, {0, 80});
	CHECK(split[0] == 100 && split[1] == 80);
	split = PreviewLayout::Distribute(150, {0, 100}, {0, 0});
	CHECK(split[0] == 0 && split[1] == 150);
	split = PreviewLayout::Distribute(150, {0, 0}, {10, 20});
	CHECK(split[0] == 10 && split[1] == 20);
	CHECK(PreviewLayout::Distribute(100, {}, {}).isEmpty());
}

static void testSqueezedPreviewIsRestored()
{
	StudioWindow studio;
	QMainWindow &window = studio.window;
	window.resize(1600, 1000);
	window.show();
	settle();

	/* Layout saved on a large monitor: tall bottom docks, wide side docks. */
	window.resizeDocks({studio.mixer, studio.transitions, studio.tools}, {400, 400, 400}, Qt::Vertical);
	window.resizeDocks({studio.scenes, studio.sources}, {420, 420}, Qt::Horizontal);
	window.resizeDocks({studio.controls}, {420}, Qt::Horizontal);
	settle();
	CHECK(window.centralWidget()->height() < 500);
	const QByteArray state = window.saveState();

	/* Same state restored on a small laptop screen. */
	window.resize(1100, 650);
	settle();
	window.restoreState(state);
	settle();
	const double squeezedHeight = PreviewLayout::HeightShare(&window);
	const double squeezedWidth = PreviewLayout::WidthShare(&window);
	std::cout << "restored on small screen: height share " << squeezedHeight << ", width share " << squeezedWidth
		  << std::endl;
	CHECK(squeezedHeight < PreviewLayout::kFloorHeightShare);
	CHECK(squeezedWidth < PreviewLayout::kFloorWidthShare);

	CHECK(PreviewLayout::EnsureMinimumShare(&window));
	settle();
	const double fixedHeight = PreviewLayout::HeightShare(&window);
	const double fixedWidth = PreviewLayout::WidthShare(&window);
	std::cout << "after floor: height share " << fixedHeight << ", width share " << fixedWidth << std::endl;
	CHECK(fixedHeight >= PreviewLayout::kFloorHeightShare - 0.01);
	CHECK(fixedWidth >= PreviewLayout::kFloorWidthShare - 0.01);
	/* Docks still exist and are not collapsed below their content. */
	CHECK(studio.tools->height() >= 110);
	CHECK(studio.controls->width() >= 150);

	/* A preview that already has room is left untouched. */
	const int before = window.centralWidget()->height();
	CHECK(!PreviewLayout::EnsureMinimumShare(&window));
	settle();
	CHECK(window.centralWidget()->height() == before);
}

static void testPresetsAndSteps()
{
	StudioWindow studio;
	QMainWindow &window = studio.window;
	window.resize(1400, 900);
	window.show();
	settle();

	PreviewLayout::ApplyShare(&window, PreviewLayout::kLargeHeightShare, PreviewLayout::kLargeWidthShare);
	settle();
	const double largeHeight = PreviewLayout::HeightShare(&window);
	const double largeWidth = PreviewLayout::WidthShare(&window);
	std::cout << "large preset: " << largeHeight << " x " << largeWidth << std::endl;
	CHECK(near(largeHeight, PreviewLayout::kLargeHeightShare, 0.02));
	CHECK(near(largeWidth, PreviewLayout::kLargeWidthShare, 0.02));

	PreviewLayout::ApplyShare(&window, PreviewLayout::kCompactHeightShare, PreviewLayout::kCompactWidthShare);
	settle();
	const double compactHeight = PreviewLayout::HeightShare(&window);
	const double compactWidth = PreviewLayout::WidthShare(&window);
	std::cout << "compact preset: " << compactHeight << " x " << compactWidth << std::endl;
	CHECK(near(compactHeight, PreviewLayout::kCompactHeightShare, 0.02));
	CHECK(near(compactWidth, PreviewLayout::kCompactWidthShare, 0.02));
	CHECK(compactHeight < largeHeight);

	/* Stepping up and down moves the preview by one step each time. */
	PreviewLayout::ApplyShare(&window, compactHeight + PreviewLayout::kStep, compactWidth + PreviewLayout::kStep);
	settle();
	CHECK(near(PreviewLayout::HeightShare(&window), compactHeight + PreviewLayout::kStep, 0.02));
	CHECK(near(PreviewLayout::WidthShare(&window), compactWidth + PreviewLayout::kStep, 0.02));

	/* The share survives a window resize when re-applied, which is what the
	 * main window does after restoring a saved layout. */
	window.resize(1000, 620);
	settle();
	PreviewLayout::ApplyShare(&window, PreviewLayout::kBalancedHeightShare, PreviewLayout::kBalancedWidthShare);
	settle();
	std::cout << "balanced on small window: " << PreviewLayout::HeightShare(&window) << " x "
		  << PreviewLayout::WidthShare(&window) << std::endl;
	CHECK(near(PreviewLayout::HeightShare(&window), PreviewLayout::kBalancedHeightShare, 0.03));
	CHECK(near(PreviewLayout::WidthShare(&window), PreviewLayout::kBalancedWidthShare, 0.03));
}

static void testFloatingAndHiddenDocksAreIgnored()
{
	StudioWindow studio;
	QMainWindow &window = studio.window;
	window.resize(1200, 800);
	window.show();
	settle();

	studio.mixer->setFloating(true);
	studio.transitions->hide();
	studio.tools->setFloating(true);
	settle();
	CHECK(PreviewLayout::DocksInArea(&window, Qt::BottomDockWidgetArea).isEmpty());
	CHECK(!PreviewLayout::ApplyHeightShare(&window, 0.5));
	CHECK(PreviewLayout::DocksInArea(&window, Qt::LeftDockWidgetArea).size() == 2);
	CHECK(PreviewLayout::ApplyWidthShare(&window, 0.5));
}

int main(int argc, char **argv)
{
	QApplication app(argc, argv);
	testMath();
	testSqueezedPreviewIsRestored();
	testPresetsAndSteps();
	testFloatingAndHiddenDocksAreIgnored();
	if (failures) {
		std::cerr << failures << " check(s) failed" << std::endl;
		return 1;
	}
	std::cout << "preview layout tests passed" << std::endl;
	return 0;
}
