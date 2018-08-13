#include "area_window.hpp"

#include <Windows.h>

namespace area_window {

struct AreaData {
	OBSSceneItem item;
	OBSSignal transformSignal;

	HWND window{};
	int width{};
	int height{};
	obs_sceneitem_crop rect{};
};

static AreaData area;

#define AREA_WINDOW_CLASS_NAME L"OBS_AREA_WINDOW"
#define AREA_WINDOW_COLOR_KEY RGB(0xFF, 0x20, 0xFF)

static void paint_window()
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

static LRESULT CALLBACK window_proc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
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

static void create_area_class()
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

static void create_window()
{
	create_area_class();
	const DWORD exStyle = WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST | WS_EX_NOACTIVATE;
	const DWORD style = WS_POPUP;
	const BOOL hasMenu = FALSE;

	RECT rect;
	rect.left = area.rect.left;
	rect.top = area.rect.top;
	rect.right = area.width - area.rect.right;
	rect.bottom = area.height - area.rect.bottom;
	AdjustWindowRectEx(&rect, style, hasMenu, exStyle);
	int width = 4 + rect.right - rect.left;
	int height = 4 + rect.bottom - rect.top;
	int x = rect.left - 2;
	int y = rect.top - 2;

	const auto windowName = nullptr;
	const HWND parentWindow = nullptr;
	const auto menu = HMENU{};
	const auto customParam = nullptr;
	const HINSTANCE instance = GetModuleHandle(nullptr);
	const HWND hwnd = CreateWindowExW(
				exStyle,
				AREA_WINDOW_CLASS_NAME,
				windowName,
				style,
				x,
				y,
				width,
				height,
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

static void destroy_window()
{
	if (!area.window) return;
	DestroyWindow(area.window);
	area.window = nullptr;
}

} // namespace area_window

void AreaWindow::update()
{
	using namespace area_window;

	auto source = obs_sceneitem_get_source(area.item);
	auto sourceWidth = obs_source_get_width(source);
	auto sourceHeight = obs_source_get_height(source);

	if (sourceWidth == 0 || sourceHeight == 0) return;

	struct obs_sceneitem_crop crop;
	obs_sceneitem_get_crop(area.item, &crop);

	if (!area.window) {
		area.width = sourceWidth;
		area.height = sourceHeight;
		memcpy(&area.rect, &crop, sizeof(crop));

		create_window();
	}
	else if (sourceWidth != area.width || sourceHeight != area.height ||
			 crop.left != area.rect.left || crop.top != area.rect.top ||
			 crop.right != area.rect.right || crop.bottom != area.rect.bottom) {

		area.width = sourceWidth;
		area.height = sourceHeight;
		memcpy(&area.rect, &crop, sizeof(crop));

		int width = area.width - crop.left - crop.right;
		int height = area.height - crop.top - crop.bottom;
		bool repaint = true;
		MoveWindow(area.window, crop.left - 2, crop.top - 2, width + 4, height + 4, repaint);
	}
}

static void OnItemTransform(void *param, calldata_t *data) {
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
