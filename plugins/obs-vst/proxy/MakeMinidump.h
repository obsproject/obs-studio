#pragma once

#include "client/crash_report_database.h"
#include "client/crashpad_client.h"
#include "client/settings.h"

#include <shlobj_core.h>

class CrashHandler
{
public:
	bool start(const std::string& reportServerUrl)
	{
		PWSTR   ppszPath;	
		HRESULT hResult = SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, NULL, &ppszPath);

		if (hResult != S_OK)
			return false;
	
		std::wstring appdata_path(ppszPath);
		appdata_path.append(L"\\obs-studio-node-server");
	
		CoTaskMemFree(ppszPath);

		m_arguments.push_back("--no-rate-limit");

		std::wstring handler_path(L"crashpad_handler.exe");

		m_db = base::FilePath(appdata_path);
		m_handler = base::FilePath(handler_path);
		m_database = crashpad::CrashReportDatabase::Initialize(m_db);

		if (m_database == nullptr || m_database->GetSettings() == nullptr)
			return false;

		m_database->GetSettings()->SetUploadsEnabled(true);

		bool rc = m_client.StartHandler(m_handler, m_db, m_db, reportServerUrl, m_annotations, m_arguments, true, true);

		if (!rc)
			return false;

		rc = m_client.WaitForHandlerStart(INFINITE);

		if (!rc)
			return false;

		return true;
	}

private:
	crashpad::CrashpadClient m_client;
	std::unique_ptr<crashpad::CrashReportDatabase> m_database;
	base::FilePath m_db;
	std::map<std::string, std::string> m_annotations;
	std::vector<std::string> m_arguments;
	base::FilePath m_handler;
};
