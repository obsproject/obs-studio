/*****************************************************************************
Copyright (C) 2016-2017 by Colin Edwards.
Additional Code Copyright (C) 2016-2017 by c3r1c3 <c3r1c3@nevermindonline.com>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*****************************************************************************/
#include "../headers/VSTPlugin.h"
#include "VstWinDefs.h"

#include <obs_vst_api.grpc.pb.h>

#include <util/platform.h>
#include <windows.h>
#include <string>
#include <grpcpp/grpcpp.h>
#include <filesystem>

#include "../headers/grpc_vst_communicatorClient.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

AEffect* VSTPlugin::loadEffect()
{
	blog(LOG_DEBUG, "VST Plug-in: starting win-streamlabs-vst.exe for '%s'", m_pluginPath.c_str());

	wchar_t *wpath;
	os_utf8_to_wcs_ptr(m_pluginPath.c_str(), 0, &wpath);

	const int32_t portNumber = chooseProxyPort();

	STARTUPINFOW si;
	memset(&si, NULL, sizeof(si));
	si.cb = sizeof(si);

	m_effect = std::make_unique<AEffect>();
	std::wstring startparams = L"streamlabs_vst.exe \"" + std::wstring(wpath) + L"\" " + std::to_wstring(portNumber) + L" " + std::to_wstring(GetCurrentProcessId());

	BOOL launched = FALSE;
	try {
		const char *module_path = obs_get_module_binary_path(obs_current_module());
		if (!module_path)
			return nullptr;

		std::wstring process_path = std::filesystem::u8path(module_path).remove_filename().wstring() + L"/win-streamlabs-vst.exe";

		launched = CreateProcessW(process_path.c_str(), (LPWSTR)startparams.c_str(), NULL, NULL, FALSE, CREATE_NEW_CONSOLE, NULL, NULL, &si, &m_winServer);
	} catch (...) {
		blog(LOG_ERROR, "VST Plug-in: Crashed while launching vst server");
	}
	if (!launched)
	{
		::MessageBoxA(NULL, (std::filesystem::path(m_pluginPath).filename().string() + " failed to launch.\n\n You may restart the application or recreate the filter to try again.").c_str(), "VST Filter Error",
			MB_ICONERROR | MB_TOPMOST);

		blog(LOG_ERROR, "VST Plug-in: can't start vst server, GetLastError = %d", GetLastError());
		m_effect = nullptr;
		return nullptr;
	}

	m_remote = std::make_unique<grpc_vst_communicatorClient>(grpc::CreateChannel("localhost:" + std::to_string(portNumber), grpc::InsecureChannelCredentials()));
	m_remote->updateAEffect(m_effect.get());

	if (!verifyProxy())
		return nullptr;

	return m_effect.get();
}

int32_t VSTPlugin::chooseProxyPort()
{
	int32_t result = 0;

#ifdef WIN32
	struct sockaddr_in local;
	SOCKET sockt = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);

	if (sockt == INVALID_SOCKET)
		return result;

	local.sin_addr.s_addr = htonl(INADDR_ANY);
	local.sin_family = AF_INET;
	local.sin_port = htons(0);

	// Bind
	if (::bind(sockt, (struct sockaddr*)&local, sizeof(local)) == SOCKET_ERROR)
		return result;
	
	struct sockaddr_in my_addr;
	memset(&my_addr, NULL, sizeof(my_addr));
	int len = sizeof(my_addr);
	getsockname(sockt, (struct sockaddr *) &my_addr, &len);
	result = ntohs(my_addr.sin_port);

	closesocket(sockt);
#endif
	return result;
}

void VSTPlugin::stopProxy()
{
	if (m_effect == nullptr)
		return;

	auto movedPtr = move(m_effect);

	if (m_remote == nullptr)
		return;
	
	m_remote->stopServer(movedPtr.get());
	
	// Wait for graceful end in a thread, don't block here
	std::thread([](HANDLE hProcess, HANDLE hThread, INT nWaitTime) {
		
		// Might have to kill it, wait a moment but note that wait time is 0 if tcp connection already isn't valid
		if (WaitForSingleObject(hProcess, nWaitTime) == WAIT_TIMEOUT) {
			if (TerminateProcess(hProcess, 0) == FALSE) {
				blog(LOG_ERROR, "VST Plug-in: process is stuck somehow cannot terminate, GetLastError = %d", GetLastError());
			}
		}

		CloseHandle(hProcess);
		CloseHandle(hThread);

	}, m_winServer.hProcess,
	   m_winServer.hThread,
	   m_proxyDisconnected ? 3000 : 0).detach();

	m_winServer = {};
}
