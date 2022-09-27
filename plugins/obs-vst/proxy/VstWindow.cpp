#include "VstWindow.h"

#include "..\vst_header\aeffectx.h"

#include <assert.h>

VstWindow::VstWindow(AEffect* afx) :
	m_effect(afx)
{

}

VstWindow::~VstWindow()
{
	// Application is shutting down at this point
	// 
	//if (m_hwnd != NULL)
	//	::DestroyWindow(m_hwnd);
}

void VstWindow::init()
{
	if (m_hwnd != NULL)
		return;

	auto EffectWindowProc = [](HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		if (!hWnd)
			return static_cast<LRESULT>(NULL);

		if (uMsg == WM_SYSCOMMAND && wParam == SC_CLOSE)
			return static_cast<LRESULT>(NULL);

		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	};
	
	WNDCLASSEXW wcex{ sizeof(wcex) };
	wcex.lpfnWndProc = EffectWindowProc;
	wcex.hInstance = GetModuleHandleW(nullptr);
	wcex.lpszClassName = L"Minimal VST host - Guest VST Window Frame";
	RegisterClassExW(&wcex);

	LONG style = WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_DLGFRAME | WS_POPUP | WS_SYSMENU | WS_CAPTION | WS_MINIMIZEBOX;
	LONG exStyle = WS_EX_DLGMODALFRAME;

	HWND hwnd = CreateWindowEx(exStyle, wcex.lpszClassName, TEXT(""), style, 0, 0, 0, 0, nullptr, nullptr, nullptr, nullptr);

	if (hwnd == NULL)
	{
		//;error
		return;
	}

	VstRect* vstRect = nullptr;

	::EnableMenuItem(GetSystemMenu(hwnd, FALSE), SC_CLOSE, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);

	m_effect->dispatcher(m_effect, effEditOpen, 0, 0, hwnd, 0);	
	m_effect->dispatcher(m_effect, effEditGetRect, 0, 0, &vstRect, 0);
	
	::ShowWindow(hwnd, SW_SHOW);
	::ShowWindow(hwnd, SW_HIDE);
	::ShowWindow(hwnd, SW_RESTORE);

	if (vstRect) {
		::SetWindowPos(hwnd, 0, vstRect->left, vstRect->top, vstRect->right, vstRect->bottom, 0);
	}

	RECT rect = {0};
	RECT border = {0};

	if (vstRect) {
		RECT clientRect;

		::GetWindowRect(hwnd, &rect);
		::GetClientRect(hwnd, &clientRect);

		border.left  = clientRect.left - rect.left;
		border.right = rect.right - clientRect.right;

		border.top    = clientRect.top - rect.top;
		border.bottom = rect.bottom - clientRect.bottom;
	}

	/* Despite the use of AdjustWindowRectEx here, the docs lie to us
	   when they say it will give us the smallest size to accomodate the
	   client area wanted. Since Vista came out, we now have to take into
	   account hidden borders introduced by DWM. This can only be done
	   *after* we've created the window as well. */
	rect.left -= border.left;
	rect.top -= border.top;
	rect.right += border.left + border.right;
	rect.bottom += border.top + border.bottom;

	::SetWindowPos(hwnd, 0, rect.left, rect.top, rect.right, rect.bottom, 0);
	::SetWindowPos(hwnd, 0, 0, 0, 0, 0, SWP_NOSIZE);
		
	::ShowWindow(hwnd, SW_SHOW);
	::ShowWindow(hwnd, SW_HIDE);
	::ShowWindow(hwnd, SW_RESTORE);

	//setWindowTitle(plugin->effectName);

	m_hwnd = hwnd;
}

void VstWindow::update()
{	
	MSG msg;

	if (!::PeekMessage(&msg, NULL, NULL, NULL, PM_REMOVE))
		return;

	switch (msg.message)
	{
		case VstProxy::WM_USER_SHOW:
		{
			::ShowWindow(m_hwnd, SW_SHOW);
			::ShowWindow(m_hwnd, SW_HIDE);
			::ShowWindow(m_hwnd, SW_RESTORE);
			break;
		}
		case VstProxy::WM_USER_HIDE:
		{
			::ShowWindow(m_hwnd, SW_HIDE);
			break;
		}
		case VstProxy::WM_USER_CLOSE:
		{
			::DestroyWindow(m_hwnd);
			m_hwnd = NULL;
			m_effect->dispatcher(m_effect, effEditClose, 0, 0, nullptr, 0);
			break;
		}
		case VstProxy::WM_USER_SETCHUNK:
		{
			assert(0);
			break;
		}
	}

	TranslateMessage(&msg);
	DispatchMessage(&msg);
}

void VstWindow::sendMsg(const VstProxy::WM_USER_MSG msg)
{
	if (msg == VstProxy::WM_USER_MSG::WM_USER_CREATE_WINDOW)
		init();

	if (m_hwnd != NULL)
		::PostMessage(NULL, static_cast<UINT>(msg), NULL, NULL);
}
