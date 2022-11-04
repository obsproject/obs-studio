#include "update-helpers.hpp"
#include "shared-update.hpp"
#include "update-window.hpp"
#include "remote-text.hpp"
#include "qt-wrappers.hpp"
#include "win-update.hpp"
#include "obs-app.hpp"

#include <QMessageBox>

#include <string>
#include <mutex>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>

#include <util/windows/WinHandle.hpp>
#include <util/util.hpp>
#include <json11.hpp>

#ifdef BROWSER_AVAILABLE
#include <browser-panel.hpp>
#endif

using namespace std;
using namespace json11;

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

#if defined(OBS_RELEASE_CANDIDATE) && OBS_RELEASE_CANDIDATE > 0
#define CUR_VER                                                               \
	((uint64_t)OBS_RELEASE_CANDIDATE_VER << 16ULL | OBS_RELEASE_CANDIDATE \
								<< 8ULL)
#define PRE_RELEASE true
#elif OBS_BETA > 0
#define CUR_VER ((uint64_t)OBS_BETA_VER << 16ULL | OBS_BETA)
#define PRE_RELEASE true
#elif defined(OBS_COMMIT)
#define CUR_VER 1 << 16ULL
#define CUR_COMMIT OBS_COMMIT
#define PRE_RELEASE true
#else
#define CUR_VER ((uint64_t)LIBOBS_API_VER << 16ULL)
#define PRE_RELEASE false
#endif

#ifndef CUR_COMMIT
#define CUR_COMMIT "00000000"
#endif

static bool ParseUpdateManifest(const char *manifest, bool *updatesAvailable,
				string &notes_str, uint64_t &updateVer,
				string &branch)
try {

	string error;
	Json root = Json::parse(manifest, error);
	if (!error.empty())
		throw strprintf("Failed reading json string: %s",
				error.c_str());

	if (!root.is_object())
		throw string("Root of manifest is not an object");

	int major = root["version_major"].int_value();
	int minor = root["version_minor"].int_value();
	int patch = root["version_patch"].int_value();
	int rc = root["rc"].int_value();
	int beta = root["beta"].int_value();
	string commit_hash = root["commit"].string_value();

	if (major == 0 && commit_hash.empty())
		throw strprintf("Invalid version number: %d.%d.%d", major,
				minor, patch);

	const Json &notes = root["notes"];
	if (!notes.is_string())
		throw string("'notes' value invalid");

	notes_str = notes.string_value();

	const Json &packages = root["packages"];
	if (!packages.is_array())
		throw string("'packages' value invalid");

	uint64_t cur_ver;
	uint64_t new_ver;

	if (commit_hash.empty()) {
		cur_ver = CUR_VER;
		new_ver = MAKE_SEMANTIC_VERSION(
			(uint64_t)major, (uint64_t)minor, (uint64_t)patch);
		new_ver <<= 16;
		/* RC builds are shifted so that rc1 and beta1 versions do not result
		 * in the same new_ver. */
		if (rc > 0)
			new_ver |= (uint64_t)rc << 8;
		else if (beta > 0)
			new_ver |= (uint64_t)beta;
	} else {
		/* Test or nightly builds may not have a (valid) version number,
		 * so compare commit hashes instead. */
		cur_ver = stoul(CUR_COMMIT, nullptr, 16);
		new_ver = stoul(commit_hash.substr(0, 8), nullptr, 16);
	}

	updateVer = new_ver;

	/* When using a pre-release build or non-default branch we only check if
	 * the manifest version is different, so that it can be rolled-back. */
	if (branch != WIN_DEFAULT_BRANCH || PRE_RELEASE)
		*updatesAvailable = new_ver != cur_ver;
	else
		*updatesAvailable = new_ver > cur_ver;

	return true;

} catch (string &text) {
	blog(LOG_WARNING, "%s: %s", __FUNCTION__, text.c_str());
	return false;
}

#undef CUR_COMMIT
#undef CUR_VER
#undef PRE_RELEASE

/* ------------------------------------------------------------------------ */

bool GetBranchAndUrl(string &selectedBranch, string &manifestUrl)
{
	const char *config_branch =
		config_get_string(GetGlobalConfig(), "General", "UpdateBranch");
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
				manifestUrl += "manifest_" +
					       branch.name.toStdString() +
					       ".json";
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
	QMetaObject::invokeMethod(this, "infoMsg", Qt::BlockingQueuedConnection,
				  Q_ARG(QString, title), Q_ARG(QString, text));
}

int AutoUpdateThread::queryUpdateSlot(bool localManualUpdate,
				      const QString &text)
{
	OBSUpdate updateDlg(App()->GetMainWindow(), localManualUpdate, text);
	return updateDlg.exec();
}

int AutoUpdateThread::queryUpdate(bool localManualUpdate, const char *text_utf8)
{
	int ret = OBSUpdate::No;
	QString text = text_utf8;
	QMetaObject::invokeMethod(this, "queryUpdateSlot",
				  Qt::BlockingQueuedConnection,
				  Q_RETURN_ARG(int, ret),
				  Q_ARG(bool, localManualUpdate),
				  Q_ARG(QString, text));
	return ret;
}

bool AutoUpdateThread::queryRepairSlot()
{
	QMessageBox::StandardButton res = OBSMessageBox::question(
		App()->GetMainWindow(), QTStr("Updater.RepairConfirm.Title"),
		QTStr("Updater.RepairConfirm.Text"),
		QMessageBox::Yes | QMessageBox::Cancel);

	return res == QMessageBox::Yes;
}

bool AutoUpdateThread::queryRepair()
{
	bool ret = false;
	QMetaObject::invokeMethod(this, "queryRepairSlot",
				  Qt::BlockingQueuedConnection,
				  Q_RETURN_ARG(bool, ret));
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
		inline ~FinishedTrigger()
		{
			QMetaObject::invokeMethod(App()->GetMainWindow(),
						  "updateCheckFinished");
		}
	} finishedTrigger;

	/* ----------------------------------- *
	 * get branches from server            */

	if (FetchAndVerifyFile("branches", "obs-studio\\updates\\branches.json",
			       WIN_BRANCHES_URL, &text))
		App()->SetBranchData(text);

	/* ----------------------------------- *
	 * check branch and get manifest url   */

	if (!GetBranchAndUrl(branch, manifestUrl)) {
		config_set_string(GetGlobalConfig(), "General", "UpdateBranch",
				  WIN_DEFAULT_BRANCH);
		info(QTStr("Updater.BranchNotFound.Title"),
		     QTStr("Updater.BranchNotFound.Text"));
	}

	/* allow server to know if this was a manual update check in case
	 * we want to allow people to bypass a configured rollout rate */
	if (manualUpdate)
		extraHeaders.emplace_back("X-OBS2-ManualUpdate: 1");

	/* ----------------------------------- *
	 * get manifest from server            */

	text.clear();
	if (!FetchAndVerifyFile("manifest",
				"obs-studio\\updates\\manifest.json",
				manifestUrl.c_str(), &text, extraHeaders))
		return;

	/* ----------------------------------- *
	 * check manifest for update           */

	string notes;
	uint64_t updateVer = 0;

	if (!ParseUpdateManifest(text.c_str(), &updatesAvailable, notes,
				 updateVer, branch))
		throw string("Failed to parse manifest");

	if (!updatesAvailable && !repairMode) {
		if (manualUpdate)
			info(QTStr("Updater.NoUpdatesAvailable.Title"),
			     QTStr("Updater.NoUpdatesAvailable.Text"));
		return;
	} else if (updatesAvailable && repairMode) {
		info(QTStr("Updater.RepairButUpdatesAvailable.Title"),
		     QTStr("Updater.RepairButUpdatesAvailable.Text"));
		return;
	}

	/* ----------------------------------- *
	 * skip this version if set to skip    */

	uint64_t skipUpdateVer = config_get_uint(GetGlobalConfig(), "General",
						 "SkipUpdateVersion");
	if (!manualUpdate && updateVer == skipUpdateVer && !repairMode)
		return;

	/* ----------------------------------- *
	 * fetch updater module                */

	if (!FetchAndVerifyFile("updater", "obs-studio\\updates\\updater.exe",
				WIN_UPDATER_URL, nullptr))
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
				config_set_int(GetGlobalConfig(), "General",
					       "LastUpdateCheck", t);
			}
			return;

		} else if (queryResult == OBSUpdate::Skip) {
			config_set_uint(GetGlobalConfig(), "General",
					"SkipUpdateVersion", updateVer);
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

	BPtr<char> updateFilePath =
		GetConfigPathPtr("obs-studio\\updates\\updater.exe");
	BPtr<wchar_t> wUpdateFilePath;

	size_t size = os_utf8_to_wcs_ptr(updateFilePath, 0, &wUpdateFilePath);
	if (!size)
		throw string("Could not convert updateFilePath to wide");

	/* note, can't use CreateProcess to launch as admin. */
	SHELLEXECUTEINFO execInfo = {};

	execInfo.cbSize = sizeof(execInfo);
	execInfo.lpFile = wUpdateFilePath;

	string parameters = "";
	if (App()->IsPortableMode())
		parameters += "--portable";
	if (branch != WIN_DEFAULT_BRANCH)
		parameters += "--branch=" + branch;

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
		throw strprintf("Can't launch updater '%s': %d",
				updateFilePath.Get(), GetLastError());
	}

	/* force OBS to perform another update check immediately after updating
	 * in case of issues with the new version */
	config_set_int(GetGlobalConfig(), "General", "LastUpdateCheck", 0);
	config_set_int(GetGlobalConfig(), "General", "SkipUpdateVersion", 0);

	QMetaObject::invokeMethod(App()->GetMainWindow(), "close");

} catch (string &text) {
	blog(LOG_WARNING, "%s: %s", __FUNCTION__, text.c_str());
}
