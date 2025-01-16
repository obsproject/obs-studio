#include "AutoUpdateThread.hpp"
#include "ui_OBSUpdate.h"

#include <OBSApp.hpp>
#include <dialogs/OBSUpdate.hpp>
#include <updater/manifest.hpp>
#include <utility/WhatsNewInfoThread.hpp>
#include <utility/update-helpers.hpp>

#include <qt-wrappers.hpp>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>

#include "moc_AutoUpdateThread.cpp"

/* ------------------------------------------------------------------------ */

#ifndef WIN_MANIFEST_URL
#define WIN_MANIFEST_URL "https://obsproject.com/update_studio/manifest.json"
#endif

#ifndef WIN_MANIFEST_BASE_URL
#define WIN_MANIFEST_BASE_URL "https://obsproject.com/update_studio/"
#endif

#ifndef WIN_BRANCHES_URL
#define WIN_BRANCHES_URL "https://obsproject.com/update_studio/branches.json"
#endif

#ifndef WIN_DEFAULT_BRANCH
#define WIN_DEFAULT_BRANCH "stable"
#endif

#ifndef WIN_UPDATER_URL
#define WIN_UPDATER_URL "https://obsproject.com/update_studio/updater.exe"
#endif

/* ------------------------------------------------------------------------ */

using namespace std;
using namespace updater;

extern char *GetConfigPathPtr(const char *name);

static bool ParseUpdateManifest(const char *manifest_data, bool *updatesAvailable, string &notes, string &updateVer,
				const string &branch)
try {
	constexpr uint64_t currentVersion = (uint64_t)LIBOBS_API_VER << 16ULL | OBS_RELEASE_CANDIDATE << 8ULL |
					    OBS_BETA;
	constexpr bool isPreRelease = currentVersion & 0xffff || std::char_traits<char>::length(OBS_COMMIT);

	json manifestContents = json::parse(manifest_data);
	Manifest manifest = manifestContents.get<Manifest>();

	if (manifest.version_major == 0 && manifest.commit.empty())
		throw strprintf("Invalid version number: %d.%d.%d", manifest.version_major, manifest.version_minor,
				manifest.version_patch);

	notes = manifest.notes;

	if (manifest.commit.empty()) {
		uint64_t new_ver = MAKE_SEMANTIC_VERSION((uint64_t)manifest.version_major,
							 (uint64_t)manifest.version_minor,
							 (uint64_t)manifest.version_patch);
		new_ver <<= 16;
		/* RC builds are shifted so that rc1 and beta1 versions do not result
		 * in the same new_ver. */
		if (manifest.rc > 0)
			new_ver |= (uint64_t)manifest.rc << 8;
		else if (manifest.beta > 0)
			new_ver |= (uint64_t)manifest.beta;

		updateVer = to_string(new_ver);

		/* When using a pre-release build or non-default branch we only check if
		 * the manifest version is different, so that it can be rolled back. */
		if (branch != WIN_DEFAULT_BRANCH || isPreRelease)
			*updatesAvailable = new_ver != currentVersion;
		else
			*updatesAvailable = new_ver > currentVersion;
	} else {
		/* Test or nightly builds may not have a (valid) version number,
		 * so compare commit hashes instead. */
		updateVer = manifest.commit.substr(0, 8);
		*updatesAvailable = !currentVersion || !manifest.commit.compare(0, strlen(OBS_COMMIT), OBS_COMMIT);
	}

	return true;

} catch (string &text) {
	blog(LOG_WARNING, "%s: %s", __FUNCTION__, text.c_str());
	return false;
}

/* ------------------------------------------------------------------------ */

bool GetBranchAndUrl(string &selectedBranch, string &manifestUrl)
{
	const char *config_branch = config_get_string(App()->GetAppConfig(), "General", "UpdateBranch");
	if (!config_branch)
		return true;

	bool found = false;
	for (const UpdateBranch &branch : App()->GetBranches()) {
		if (branch.name != config_branch)
			continue;
		/* A branch that is found but disabled will just silently fall back to
		 * the default. But if the branch was removed entirely, the user should
		 * be warned, so leave this false *only* if the branch was removed. */
		found = true;

		if (branch.is_enabled) {
			selectedBranch = branch.name.toStdString();
			if (branch.name != WIN_DEFAULT_BRANCH) {
				manifestUrl = WIN_MANIFEST_BASE_URL;
				manifestUrl += "manifest_" + branch.name.toStdString() + ".json";
			}
		}
		break;
	}

	return found;
}

/* ------------------------------------------------------------------------ */

void AutoUpdateThread::infoMsg(const QString &title, const QString &text)
{
	OBSMessageBox::information(App()->GetMainWindow(), title, text);
}

void AutoUpdateThread::info(const QString &title, const QString &text)
{
	QMetaObject::invokeMethod(this, "infoMsg", Qt::BlockingQueuedConnection, Q_ARG(QString, title),
				  Q_ARG(QString, text));
}

int AutoUpdateThread::queryUpdateSlot(bool localManualUpdate, const QString &text)
{
	OBSUpdate updateDlg(App()->GetMainWindow(), localManualUpdate, text);
	return updateDlg.exec();
}

int AutoUpdateThread::queryUpdate(bool localManualUpdate, const char *text_utf8)
{
	int ret = OBSUpdate::No;
	QString text = text_utf8;
	QMetaObject::invokeMethod(this, "queryUpdateSlot", Qt::BlockingQueuedConnection, Q_RETURN_ARG(int, ret),
				  Q_ARG(bool, localManualUpdate), Q_ARG(QString, text));
	return ret;
}

bool AutoUpdateThread::queryRepairSlot()
{
	QMessageBox::StandardButton res =
		OBSMessageBox::question(App()->GetMainWindow(), QTStr("Updater.RepairConfirm.Title"),
					QTStr("Updater.RepairConfirm.Text"), QMessageBox::Yes | QMessageBox::Cancel);

	return res == QMessageBox::Yes;
}

bool AutoUpdateThread::queryRepair()
{
	bool ret = false;
	QMetaObject::invokeMethod(this, "queryRepairSlot", Qt::BlockingQueuedConnection, Q_RETURN_ARG(bool, ret));
	return ret;
}

void AutoUpdateThread::run()
try {
	string text;
	string branch = WIN_DEFAULT_BRANCH;
	string manifestUrl = WIN_MANIFEST_URL;
	vector<string> extraHeaders;
	bool updatesAvailable = false;

	struct FinishedTrigger {
		inline ~FinishedTrigger() { QMetaObject::invokeMethod(App()->GetMainWindow(), "updateCheckFinished"); }
	} finishedTrigger;

	/* ----------------------------------- *
	 * get branches from server            */

	if (FetchAndVerifyFile("branches", "obs-studio\\updates\\branches.json", WIN_BRANCHES_URL, &text))
		App()->SetBranchData(text);

	/* ----------------------------------- *
	 * check branch and get manifest url   */

	if (!GetBranchAndUrl(branch, manifestUrl)) {
		config_set_string(App()->GetAppConfig(), "General", "UpdateBranch", WIN_DEFAULT_BRANCH);
		info(QTStr("Updater.BranchNotFound.Title"), QTStr("Updater.BranchNotFound.Text"));
	}

	/* allow server to know if this was a manual update check in case
	 * we want to allow people to bypass a configured rollout rate */
	if (manualUpdate)
		extraHeaders.emplace_back("X-OBS2-ManualUpdate: 1");

	/* ----------------------------------- *
	 * get manifest from server            */

	text.clear();
	if (!FetchAndVerifyFile("manifest", "obs-studio\\updates\\manifest.json", manifestUrl.c_str(), &text,
				extraHeaders))
		return;

	/* ----------------------------------- *
	 * check manifest for update           */

	string notes;
	string updateVer;

	if (!ParseUpdateManifest(text.c_str(), &updatesAvailable, notes, updateVer, branch))
		throw string("Failed to parse manifest");

	if (!updatesAvailable && !repairMode) {
		if (manualUpdate)
			info(QTStr("Updater.NoUpdatesAvailable.Title"), QTStr("Updater.NoUpdatesAvailable.Text"));
		return;
	} else if (updatesAvailable && repairMode) {
		info(QTStr("Updater.RepairButUpdatesAvailable.Title"), QTStr("Updater.RepairButUpdatesAvailable.Text"));
		return;
	}

	/* ----------------------------------- *
	 * skip this version if set to skip    */

	const char *skipUpdateVer = config_get_string(App()->GetAppConfig(), "General", "SkipUpdateVersion");
	if (!manualUpdate && !repairMode && skipUpdateVer && updateVer == skipUpdateVer)
		return;

	/* ----------------------------------- *
	 * fetch updater module                */

	if (!FetchAndVerifyFile("updater", "obs-studio\\updates\\updater.exe", WIN_UPDATER_URL, nullptr))
		return;

	/* ----------------------------------- *
	 * query user for update               */

	if (repairMode) {
		if (!queryRepair())
			return;
	} else {
		int queryResult = queryUpdate(manualUpdate, notes.c_str());

		if (queryResult == OBSUpdate::No) {
			if (!manualUpdate) {
				long long t = (long long)time(nullptr);
				config_set_int(App()->GetAppConfig(), "General", "LastUpdateCheck", t);
			}
			return;

		} else if (queryResult == OBSUpdate::Skip) {
			config_set_string(App()->GetAppConfig(), "General", "SkipUpdateVersion", updateVer.c_str());
			return;
		}
	}

	/* ----------------------------------- *
	 * get working dir                     */

	wchar_t cwd[MAX_PATH];
	GetModuleFileNameW(nullptr, cwd, _countof(cwd) - 1);
	wchar_t *p = wcsrchr(cwd, '\\');
	if (p)
		*p = 0;

	/* ----------------------------------- *
	 * execute updater                     */

	BPtr<char> updateFilePath = GetAppConfigPathPtr("obs-studio\\updates\\updater.exe");
	BPtr<wchar_t> wUpdateFilePath;

	size_t size = os_utf8_to_wcs_ptr(updateFilePath, 0, &wUpdateFilePath);
	if (!size)
		throw string("Could not convert updateFilePath to wide");

	/* note, can't use CreateProcess to launch as admin. */
	SHELLEXECUTEINFO execInfo = {};

	execInfo.cbSize = sizeof(execInfo);
	execInfo.lpFile = wUpdateFilePath;

	string parameters;
	if (branch != WIN_DEFAULT_BRANCH)
		parameters += "--branch=" + branch;

	obs_cmdline_args obs_args = obs_get_cmdline_args();
	for (int idx = 1; idx < obs_args.argc; idx++) {
		if (!parameters.empty())
			parameters += " ";

		parameters += obs_args.argv[idx];
	}

	/* Portable mode can be enabled via sentinel files, so copying the
	 * command line doesn't guarantee the flag to be there. */
	if (App()->IsPortableMode() && parameters.find("--portable") == string::npos) {
		if (!parameters.empty())
			parameters += " ";
		parameters += "--portable";
	}

	BPtr<wchar_t> lpParameters;
	size = os_utf8_to_wcs_ptr(parameters.c_str(), 0, &lpParameters);
	if (!size && !parameters.empty())
		throw string("Could not convert parameters to wide");

	execInfo.lpParameters = lpParameters;
	execInfo.lpDirectory = cwd;
	execInfo.nShow = SW_SHOWNORMAL;

	if (!ShellExecuteEx(&execInfo)) {
		QString msg = QTStr("Updater.FailedToLaunch");
		info(msg, msg);
		throw strprintf("Can't launch updater '%s': %d", updateFilePath.Get(), GetLastError());
	}

	/* force OBS to perform another update check immediately after updating
	 * in case of issues with the new version */
	config_set_int(App()->GetAppConfig(), "General", "LastUpdateCheck", 0);
	config_set_string(App()->GetAppConfig(), "General", "SkipUpdateVersion", "0");

	QMetaObject::invokeMethod(App()->GetMainWindow(), "close");

} catch (string &text) {
	blog(LOG_WARNING, "%s: %s", __FUNCTION__, text.c_str());
}
