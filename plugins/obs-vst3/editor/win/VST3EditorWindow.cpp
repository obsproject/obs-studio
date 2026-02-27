/******************************************************************************
    Copyright (C) 2025-2026 pkv <pkv@obsproject.com>
    This file is part of obs-vst3.
    It uses the Steinberg VST3 SDK, which is licensed under MIT license.
    See https://github.com/steinbergmedia/vst3sdk for details.
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/
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
		: view_(view),
		  hwnd_(nullptr),
		  frame_(nullptr),
		  title_(title)
	{
		s_instance = this;
		resizeable_ = view->canResize() == Steinberg::kResultTrue;
	}

	~Impl()
	{
		if (hwnd_)
			SetWindowLongPtr(hwnd_, GWLP_USERDATA, 0);

		if (hwnd_) {
			DestroyWindow(hwnd_);
			hwnd_ = nullptr;
		}
		if (frame_) {
			delete frame_;
			frame_ = nullptr;
		}
		s_instance = nullptr;
		view_ = nullptr;
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
		if (resizeable_)
			dwStyle |= WS_SIZEBOX | WS_MAXIMIZEBOX;

		std::wstring windowTitleW = utf8_to_wide(title_);

		RECT rect = {0, 0, width, height};
		AdjustWindowRectEx(&rect, dwStyle, FALSE, exStyle);

		hwnd_ = CreateWindowExW(exStyle, L"VST3EditorWin32", windowTitleW.c_str(), dwStyle, CW_USEDEFAULT,
					CW_USEDEFAULT, rect.right - rect.left, rect.bottom - rect.top, nullptr, nullptr,
					GetModuleHandle(nullptr), nullptr);

		if (!hwnd_) {
			fail("VST3: Plugin window creation failed!");
			return false;
		}

		SetWindowLongPtr(hwnd_, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
		SetWindowTextW(hwnd_, windowTitleW.c_str());

		frame_ = new PlugFrameImpl(hwnd_);

		// Attach plugin view
		if (view_) {
			view_->setFrame(frame_);
			if (view_->attached((void *)hwnd_, Steinberg::kPlatformTypeHWND) != Steinberg::kResultOk) {
				view_->setFrame(nullptr);
				fail("VST3: Plugin attachment to window failed!");
				return false;
			}
			// Initial size with a hack for VST3s which wait till being attached to return their size ...
			Steinberg::ViewRect vr(0, 0, width, height);
			Steinberg::ViewRect rect;
			if (view_->getSize(&rect) == Steinberg::kResultOk) {
				if (rect.getWidth() != width || rect.getHeight() != height)
					view_->onSize(&rect);
			} else {
				view_->onSize(&vr);
			}
			// DPI
			Steinberg::FUnknownPtr<Steinberg::IPlugViewContentScaleSupport> scaleSupport(view_);
			if (scaleSupport) {
				float dpi = GetDpiForWindow(hwnd_) / 96.0f;
				scaleSupport->setContentScaleFactor(dpi);
			}
		}
		return true;
	}

	void show()
	{
		if (!hwnd_ || !view_)
			return;
		SetWindowPos(hwnd_, HWND_TOP, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOCOPYBITS | SWP_SHOWWINDOW);
		wasClosed_ = false;
	}

	void hide()
	{
		if (hwnd_)
			ShowWindow(hwnd_, SW_HIDE);
	}

	class PlugFrameImpl : public Steinberg::IPlugFrame {
	public:
		PlugFrameImpl(HWND hwnd) : hwnd_(hwnd) {}
		Steinberg::tresult PLUGIN_API resizeView(Steinberg::IPlugView *view,
							 Steinberg::ViewRect *newSize) override
		{
			if (hwnd_ && newSize) {
				RECT winRect = {0, 0, newSize->getWidth(), newSize->getHeight()};
				int style = GetWindowLong(hwnd_, GWL_STYLE);
				int exStyle = GetWindowLong(hwnd_, GWL_EXSTYLE);
				AdjustWindowRectEx(&winRect, style, FALSE, exStyle);
				SetWindowPos(hwnd_, nullptr, 0, 0, winRect.right - winRect.left,
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
		HWND hwnd_;
	};

	bool getClosedState() { return wasClosed_; }

	static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		if (!s_instance)
			return FALSE;
		if (hwnd == s_instance->hwnd_) {
			switch (msg) {
			case WM_SIZE:
				if (s_instance->view_) {
					RECT r;
					GetClientRect(hwnd, &r);
					Steinberg::ViewRect vr(0, 0, r.right - r.left, r.bottom - r.top);
					s_instance->view_->onSize(&vr);
				}
				break;
			case WM_CLOSE:
				s_instance->hide();
				s_instance->wasClosed_ = true;
				return TRUE;
			case WM_DPICHANGED:
				RECT *suggested = (RECT *)lParam;
				SetWindowPos(hwnd, nullptr, suggested->left, suggested->top,
					     suggested->right - suggested->left, suggested->bottom - suggested->top,
					     SWP_NOZORDER | SWP_NOACTIVATE);

				UINT dpi = LOWORD(wParam);
				float scale = static_cast<float>(dpi) / 96.0f;

				if (s_instance && s_instance->view_) {
					Steinberg::FUnknownPtr<Steinberg::IPlugViewContentScaleSupport> scaleSupport(
						s_instance->view_);
					if (scaleSupport) {
						scaleSupport->setContentScaleFactor(scale);
					}
					RECT r;
					GetClientRect(hwnd, &r);
					Steinberg::ViewRect vr(0, 0, r.right - r.left, r.bottom - r.top);
					s_instance->view_->onSize(&vr);
				}
				return FALSE;
			}
		}
		return DefWindowProcW(hwnd, msg, wParam, lParam);
	}

	Steinberg::IPlugView *view_;
	HWND hwnd_;
	PlugFrameImpl *frame_;
	std::string title_;
	static Impl *s_instance;
	bool resizeable_;
	bool wasClosed_ = false;
};

VST3EditorWindow::Impl *VST3EditorWindow::Impl::s_instance = nullptr;

VST3EditorWindow::VST3EditorWindow(Steinberg::IPlugView *view, const std::string &title) : impl_(new Impl(view, title))
{
}

VST3EditorWindow::~VST3EditorWindow()
{
	delete impl_;
}

bool VST3EditorWindow::create(int width, int height)
{
	return impl_->create(width, height);
}

void VST3EditorWindow::show()
{
	impl_->show();
}

void VST3EditorWindow::close()
{
	impl_->hide();
}

bool VST3EditorWindow::getClosedState()
{
	return impl_->getClosedState();
}
