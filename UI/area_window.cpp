#include "area_window.hpp"

#ifdef _WIN32
#include <Windows.h>

namespace area_window {
namespace {

struct window_sizing {
	int x{};
	int y{};
	int w{};
	int h{};

	auto operator== (const window_sizing& r) const
	{
		return x == r.x && y == r.y && w == r.w && h == r.h;
	}
	auto operator!= (const window_sizing& r) const
	{
		return !(*this == r);
	}
};

struct AreaData {
	OBSSceneItem item;
	OBSSignal transformSignal;

	HWND window{};
	window_sizing size{};
};
static AreaData area;

#define AREA_WINDOW_CLASS_NAME L"OBS_AREA_WINDOW"
#define AREA_WINDOW_COLOR_KEY RGB(0xFF, 0x20, 0xFF)

void paint_window()
{
	RECT window_rect;
	GetClientRect(area.window, &window_rect);
	int width = window_rect.right - window_rect.left;
	int height = window_rect.bottom - window_rect.top;

	PAINTSTRUCT p;
	auto dc = BeginPaint(area.window, &p);

	SelectObject(dc, GetStockObject(HOLLOW_BRUSH));
	SelectObject(dc, GetStockObject(DC_PEN));

	SetDCPenColor(dc, RGB(255, 80, 40));
	Rectangle(dc, 0, 0, width, height);

	SelectObject(dc, GetStockObject(DC_BRUSH));
	SetDCBrushColor(dc, AREA_WINDOW_COLOR_KEY);
	SetDCPenColor(dc, RGB(255, 255, 255));
	Rectangle(dc, 1, 1, width - 1, height - 1);

	EndPaint(area.window, &p);
}

LRESULT CALLBACK window_proc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message) {
	case WM_CLOSE:
		return 0;
	case WM_SIZE:
		return 0;
	case WM_MOUSEMOVE:
		return 0;
	case WM_PAINT:
		if (window == area.window) {
			paint_window();
			return 0;
		}
		break;
	}
	return DefWindowProc(window, message, wParam, lParam);
}

void create_area_class()
{
	static bool registered = false;
	if (registered) return;
	registered = true;

	WNDCLASSEXW window_class;
	window_class.cbSize = sizeof(WNDCLASSEXW);
	window_class.style = CS_HREDRAW | CS_VREDRAW;
	window_class.lpfnWndProc = &window_proc;
	window_class.cbClsExtra = 0;
	window_class.cbWndExtra = 0;
	window_class.hInstance = GetModuleHandle(NULL);
	window_class.hIcon = NULL;
	window_class.hCursor = NULL;
	window_class.hbrBackground = NULL;
	window_class.lpszMenuName = NULL;
	window_class.lpszClassName = AREA_WINDOW_CLASS_NAME;
	window_class.hIconSm = NULL;

	RegisterClassExW(&window_class);
}

void create_window()
{
	create_area_class();
	const auto& s = area.size;

	const DWORD exStyle = WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST | WS_EX_NOACTIVATE;
	const DWORD style = WS_POPUP;
	const auto windowName = nullptr;
	const auto parentWindow = HWND{};
	const auto menu = HMENU{};
	const auto customParam = nullptr;
	const auto instance = GetModuleHandle(nullptr);
	const auto hwnd = CreateWindowExW(
				exStyle,
				AREA_WINDOW_CLASS_NAME,
				windowName,
				style,
				s.x,
				s.y,
				s.w,
				s.h,
				parentWindow,
				menu,
				instance,
				customParam);
	area.window = hwnd;

	const BYTE alpha = 0u;
	const BYTE flags = LWA_COLORKEY;
	SetLayeredWindowAttributes(hwnd, AREA_WINDOW_COLOR_KEY, alpha, flags);

	ShowWindow(hwnd, SW_SHOW);
	UpdateWindow(hwnd);
}

void move_window()
{
	auto repaint = true;
	const auto& s = area.size;
	MoveWindow(area.window, s.x, s.y, s.w, s.h, repaint);
}

void destroy_window()
{
	if (!area.window) return;
	DestroyWindow(area.window);
	area.window = nullptr;
}

auto compute_window_sizing() -> window_sizing
{
	window_sizing r{};

	auto source = obs_sceneitem_get_source(area.item);
	auto sourceWidth = obs_source_get_width(source);
	auto sourceHeight = obs_source_get_height(source);

	if (sourceWidth == 0 || sourceHeight == 0) return r;

	obs_sceneitem_crop crop;
	obs_sceneitem_get_crop(area.item, &crop);

	obs_transform_info osi;
	obs_sceneitem_get_info(area.item, &osi);

	auto scene = obs_sceneitem_get_scene(area.item);
	auto sceneSource = obs_scene_get_source(scene);
	auto sceneWidth = obs_source_get_width(sceneSource);
	auto sceneHeight = obs_source_get_height(sceneSource);

	auto minX = max(0, -osi.pos.x / osi.scale.x);
	auto minY = max(0, -osi.pos.y / osi.scale.y);
	auto maxWidth = (sceneWidth) / osi.scale.x;
	auto maxHeight = (sceneHeight) / osi.scale.y;

	r.x = minX + crop.left;
	r.y = minY + crop.top;
	r.w = min(maxWidth, sourceWidth - crop.left - crop.right);
	r.h = min(maxHeight, sourceHeight - crop.top - crop.bottom);

	// we want to make sure we paint arround the area => add 2 pixels
	r.x -= 2;
	r.y -= 2;
	r.w += 4;
	r.h += 4;
	return r;
}

} // namespace
} // namespace area_window

void AreaWindow::update()
{
	using namespace area_window;

	auto size = compute_window_sizing();
	if (size.w == 0 || size.h == 0) return;

	if (!area.window) {
		area.size = size;
		create_window();
	}
	else if (size != area.size) {
		area.size = size;
		move_window();
	}
}

static void OnItemTransform(void *param, calldata_t *data)
{
	using namespace area_window;
	auto window = static_cast<AreaWindow*>(param);
	OBSSceneItem item = (obs_sceneitem_t*)calldata_ptr(data, "item");
	if (item != area.item) return;

	QMetaObject::invokeMethod(window, &AreaWindow::update);
}

void AreaWindow::start(obs_sceneitem_t *item)
{
	auto source = obs_sceneitem_get_source(item);
	if (0 != strcmp(obs_source_get_id(source), "monitor_capture")) return;

	using namespace area_window;
	if (area.item == item) return;
	if (area.item) stop();
	if (!item) return;

	area.item = OBSSceneItem(item);
	auto scene = obs_sceneitem_get_scene(item);
	auto sceneSource = obs_scene_get_source(scene);
	area.transformSignal.Connect(obs_source_get_signal_handler(sceneSource), "item_transform", OnItemTransform, this);
	// TODO: find a way to recognize that output size has changed

	update();
}

void AreaWindow::stop()
{
	using namespace area_window;
	if (!area.item) return;

	destroy_window();

	area.item = nullptr;
	area.transformSignal.Disconnect();
}

void AreaWindow::remove(obs_sceneitem_t *item)
{
	using namespace area_window;
	if (area.item == item) stop();
}

#else

// TODO: support other platforms

void AreaWindow::update() {}
void AreaWindow::start(obs_sceneitem_t *item) {}
void AreaWindow::stop() {}
void AreaWindow::remove(obs_sceneitem_t *item) {}

#endif
