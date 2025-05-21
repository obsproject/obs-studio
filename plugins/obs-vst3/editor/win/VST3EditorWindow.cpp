/* Copyright (c) 2025 pkv <pkv@obsproject.com>
 * This file is part of obs-vst3.
 *
 * This file uses the Steinberg VST3 SDK, which is licensed under MIT license.
 * See https://github.com/steinbergmedia/vst3sdk for details.
 * It also leverages demo codes from Steiberg which can be found in the sdk/samples subfolder.
 * This file and all modifications by pkv <pkv@obsproject.com> are licensed under
 * the GNU General Public License, version 3 or later, to comply with the SDK license.
 */
#include "VST3EditorWindow.h"
#include <windows.h>
#include <cassert>
#include <string>
#include <vector>
#include "pluginterfaces/gui/iplugviewcontentscalesupport.h"
#include "pluginterfaces/base/funknown.h"
#include <stdexcept>
#include <cstdlib>

#define IDI_OBSICON 101

void fail(const char *msg)
{
	fprintf(stderr, "VST3 Editor Fatal Error: %s\n", msg);
	MessageBoxA(NULL, msg, "Fatal VST3 Host Error", MB_ICONERROR | MB_OK);
	std::abort();
}

inline std::wstring utf8_to_wide(const std::string &str)
{
	if (str.empty())
		return std::wstring();
	int sz = MultiByteToWideChar(CP_UTF8, 0, str.data(), (int)str.size(), NULL, 0);
	std::wstring out(sz, 0);
	MultiByteToWideChar(CP_UTF8, 0, str.data(), (int)str.size(), &out[0], sz);
	return out;
}

HMODULE GetCurrentModule()
{
	HMODULE hModule = nullptr;
	GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, (LPCTSTR)GetCurrentModule, &hModule);
	return hModule;
}

class VST3EditorWindow::Impl {
public:
	Impl(Steinberg::IPlugView *view, const std::string &title)
		: _view(view),
		  _hwnd(nullptr),
		  _frame(nullptr),
		  _title(title)
	{
		s_instance = this;
		_resizeable = view->canResize() == Steinberg::kResultTrue;
	}

	~Impl()
	{
		if (_hwnd)
			SetWindowLongPtr(_hwnd, GWLP_USERDATA, 0);
		if (_hwnd) {
			DestroyWindow(_hwnd);
			_hwnd = nullptr;
		}
		if (_frame) {
			delete _frame;
			_frame = nullptr;
		}
		s_instance = nullptr;
		_view = nullptr;
	}

	bool create(int width, int height)
	{
		static bool classRegistered = false;
		HINSTANCE hInstance = GetModuleHandle(NULL);
		HINSTANCE hInstance2 = (HINSTANCE)GetCurrentModule();

		if (!classRegistered) {
			WNDCLASSEXW wc = {};
			wc.cbSize = sizeof(WNDCLASSEXW);
			wc.style = CS_DBLCLKS;
			wc.lpfnWndProc = WindowProc;
			wc.hInstance = hInstance;
			wc.hIcon = LoadIcon(hInstance2, MAKEINTRESOURCE(IDI_OBSICON));
			wc.hIconSm = LoadIcon(hInstance2, MAKEINTRESOURCE(IDI_OBSICON));
			wc.lpszClassName = L"VST3EditorWin32";
			wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
			wc.hbrBackground = nullptr;
			RegisterClassExW(&wc);
			classRegistered = true;
		}

		DWORD exStyle = WS_EX_APPWINDOW;
		DWORD dwStyle = WS_CAPTION | WS_SYSMENU | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_MINIMIZEBOX;
		if (_resizeable)
			dwStyle |= WS_SIZEBOX | WS_MAXIMIZEBOX;

		std::wstring windowTitleW = utf8_to_wide(_title);

		RECT rect = {0, 0, width, height};
		AdjustWindowRectEx(&rect, dwStyle, FALSE, exStyle);

		_hwnd = CreateWindowExW(exStyle, L"VST3EditorWin32", windowTitleW.c_str(), dwStyle, CW_USEDEFAULT,
					CW_USEDEFAULT, rect.right - rect.left, rect.bottom - rect.top, nullptr, nullptr,
					GetModuleHandle(nullptr), nullptr);

		if (!_hwnd) {
			fail("VST3: Plugin window creation failed!");
			return false;
		}

		SetWindowLongPtr(_hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
		SetWindowTextW(_hwnd, windowTitleW.c_str());

		_frame = new PlugFrameImpl(_hwnd);

		// Attach plugin view
		if (_view) {
			_view->setFrame(_frame);
			if (_view->attached((void *)_hwnd, Steinberg::kPlatformTypeHWND) != Steinberg::kResultOk) {
				_view->setFrame(nullptr);
				fail("VST3: Plugin attachment to window failed!");
				return false;
			}
			// Initial size with a hack for VST3s which wait till being attached to return their size ...
			Steinberg::ViewRect vr(0, 0, width, height);
			Steinberg::ViewRect rect;
			if (_view->getSize(&rect) == Steinberg::kResultOk) {
				if (rect.getWidth() != width || rect.getHeight() != height)
					_view->onSize(&rect);
			} else {
				_view->onSize(&vr);
			}
			// DPI
			Steinberg::FUnknownPtr<Steinberg::IPlugViewContentScaleSupport> scaleSupport(_view);
			if (scaleSupport) {
				float dpi = GetDpiForWindow(_hwnd) / 96.0f;
				scaleSupport->setContentScaleFactor(dpi);
			}
		}
		return true;
	}

	void show()
	{
		if (!_hwnd || !_view)
			return;
		SetWindowPos(_hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOCOPYBITS | SWP_SHOWWINDOW);
		_wasClosed = false;
	}

	void hide()
	{
		if (_hwnd)
			ShowWindow(_hwnd, SW_HIDE);
	}

	class PlugFrameImpl : public Steinberg::IPlugFrame {
	public:
		PlugFrameImpl(HWND hwnd) : _hwnd(hwnd) {}
		Steinberg::tresult PLUGIN_API resizeView(Steinberg::IPlugView *view,
							 Steinberg::ViewRect *newSize) override
		{
			if (_hwnd && newSize) {
				RECT winRect = {0, 0, newSize->getWidth(), newSize->getHeight()};
				int style = GetWindowLong(_hwnd, GWL_STYLE);
				int exStyle = GetWindowLong(_hwnd, GWL_EXSTYLE);
				AdjustWindowRectEx(&winRect, style, FALSE, exStyle);
				SetWindowPos(_hwnd, nullptr, 0, 0, winRect.right - winRect.left,
					     winRect.bottom - winRect.top, SWP_NOMOVE | SWP_NOZORDER);
				return Steinberg::kResultTrue;
			}
			return Steinberg::kResultFalse;
		}
		Steinberg::tresult PLUGIN_API queryInterface(const Steinberg::TUID _iid, void **obj) override
		{
			if (Steinberg::FUnknownPrivate::iidEqual(_iid, Steinberg::IPlugFrame::iid)) {
				*obj = this;
				return Steinberg::kResultOk;
			}
			*obj = nullptr;
			return Steinberg::kNoInterface;
		}
		// refcounting does not matter here
		uint32_t PLUGIN_API addRef() override { return 1; }
		uint32_t PLUGIN_API release() override { return 1; }

	private:
		HWND _hwnd;
	};

	bool getClosedState() { return _wasClosed; }

	static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		if (!s_instance)
			return FALSE;
		if (hwnd == s_instance->_hwnd) {
			switch (msg) {
			case WM_ERASEBKGND:
				return TRUE;
			case WM_PAINT:
				PAINTSTRUCT ps;
				BeginPaint(hwnd, &ps);
				EndPaint(hwnd, &ps);
				return FALSE;
			case WM_SIZE:
				if (s_instance->_view) {
					RECT r;
					GetClientRect(hwnd, &r);
					Steinberg::ViewRect vr(0, 0, r.right - r.left, r.bottom - r.top);
					s_instance->_view->onSize(&vr);
				}
				break;
			case WM_CLOSE:
				s_instance->hide();
				s_instance->_wasClosed = true;
				return TRUE;
			case WM_DPICHANGED:
				RECT *suggested = (RECT *)lParam;
				SetWindowPos(hwnd, nullptr, suggested->left, suggested->top,
					     suggested->right - suggested->left, suggested->bottom - suggested->top,
					     SWP_NOZORDER | SWP_NOACTIVATE);

				// Calculate new scale factor
				UINT dpi = LOWORD(wParam);
				float scale = static_cast<float>(dpi) / 96.0f;

				if (s_instance && s_instance->_view) {
					Steinberg::FUnknownPtr<Steinberg::IPlugViewContentScaleSupport> scaleSupport(
						s_instance->_view);
					if (scaleSupport) {
						scaleSupport->setContentScaleFactor(scale);
					}
					RECT r;
					GetClientRect(hwnd, &r);
					Steinberg::ViewRect vr(0, 0, r.right - r.left, r.bottom - r.top);
					s_instance->_view->onSize(&vr);
				}
				return FALSE;
			}
		}
		return DefWindowProcW(hwnd, msg, wParam, lParam);
	}

	Steinberg::IPlugView *_view;
	HWND _hwnd;
	PlugFrameImpl *_frame;
	std::string _title;
	static Impl *s_instance;
	bool _resizeable;
	bool _wasClosed = false;
};

VST3EditorWindow::Impl *VST3EditorWindow::Impl::s_instance = nullptr;

VST3EditorWindow::VST3EditorWindow(Steinberg::IPlugView *view, const std::string &title) : _impl(new Impl(view, title))
{
}

VST3EditorWindow::~VST3EditorWindow()
{
	delete _impl;
}

bool VST3EditorWindow::create(int width, int height)
{
	return _impl->create(width, height);
}

void VST3EditorWindow::show()
{
	_impl->show();
}

void VST3EditorWindow::close()
{
	_impl->hide();
}

bool VST3EditorWindow::getClosedState()
{
	return _impl->getClosedState();
}
