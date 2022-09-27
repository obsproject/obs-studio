#pragma once

#include <Windows.h>
#include <atomic>
#include <vector>
#include <string>
#include <mutex>

#include "..\win\VstWinDefs.h"

class AEffect;

class VstWindow
{
public:
	VstWindow(AEffect* afx);
	~VstWindow();

public:
	void init();
	void update();
	void sendMsg(const VstProxy::WM_USER_MSG msg);

private:
	AEffect* m_effect;
	std::atomic<HWND> m_hwnd{ NULL };

private:
	class VstRect
	{
	public:
		short top;
		short left;
		short bottom;
		short right;
	};
};
