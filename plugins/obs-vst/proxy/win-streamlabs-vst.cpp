// win-streamlabs-vst.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "VstModule.h"
#include "VstWindow.h"

#ifndef _DEBUG
	#include "MakeMinidump.h"
#endif

#include <shellapi.h>
#include <timeapi.h>

#pragma comment(lib, "Winmm.lib")

int WINAPI wWinMain(HINSTANCE /*hInstance*/, HINSTANCE /*hPrevInstance*/, PWSTR pCmdLine, int /*nCmdShow*/)
{
#ifndef _DEBUG
	CrashHandler handler;
	handler.start("https://sentry.io/api/6205131/minidump/?sentry_key=a9b80de8a8f74a8ba8d40abe453e699f");
#endif

	int argc = 0;
	LPWSTR *argv = ::CommandLineToArgvW(pCmdLine, &argc);

	if (argc < 3)
		return 0;

	::timeBeginPeriod(1);

	const std::wstring modulePath = argv[0];
	const std::wstring pipid = argv[1];
	const std::wstring ownerProcessId = argv[2];
	
	HANDLE obs64 = OpenProcess(SYNCHRONIZE, FALSE, _wtoi(ownerProcessId.c_str()));

	if (obs64 == NULL)
		return 0;

	std::mutex mtx;
	std::vector<int> outsideHwndMsgContainer;
	
	VstModule mod(modulePath, _wtoi(pipid.c_str()));
	mod.m_hwndSendFunction = [&](int msgType)
	{
		std::lock_guard<std::mutex> grd(mtx);
		outsideHwndMsgContainer.push_back(msgType);
	};

	if (!mod.start())
	{
		return 0;
	}

	VstWindow vstWindow(mod.m_effect);

	while (WaitForSingleObject(obs64, 0) == WAIT_TIMEOUT && !mod.m_stopSignal)
	{
		std::vector<int> insideMsgsCpy;

		{
			std::lock_guard<std::mutex> grd(mtx);
			insideMsgsCpy.swap(outsideHwndMsgContainer);
		}

		vstWindow.update();

		// The module's window will run on the main thread
		for (auto msg : insideMsgsCpy)
			vstWindow.sendMsg(static_cast<VstProxy::WM_USER_MSG>(msg));

		Sleep(1);
	}
	
	CloseHandle(obs64);
	mod.m_stopSignal = true;
	mod.shutdown_server();
	mod.join();
	return 0;
}
