/******************************************************************************
    Copyright (C) 2023 by Lain Bailey <lain@obsproject.com>

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
******************************************************************************/

#include <filesystem>
#include <functional>
#include <string>
#include <map>
#include <tuple>
#include <obs.hpp>
#include <util/platform.h>
#include <util/util.hpp>
#include <QMessageBox>
#include <QVariant>
#include <QFileDialog>
#include <qt-wrappers.hpp>
#include "window-basic-main.hpp"
#include "window-basic-auto-config.hpp"
#include "window-namedialog.hpp"

// MARK: Constant Expressions

constexpr std::string_view OBSProfilePath = "/obs-studio/basic/profiles/";
constexpr std::string_view OBSProfileSettingsFile = "basic.ini";

// MARK: Forward Declarations

extern void DestroyPanelCookieManager();
extern void DuplicateCurrentCookieProfile(ConfigFile &config);
extern void CheckExistingCookieId();
extern void DeleteCookies();

// MARK: - Anonymous Namespace
namespace {
QList<QString> sortedProfiles{};

void updateSortedProfiles(const OBSProfileCache &profiles)
{
	const QLocale locale = QLocale::system();
	QList<QString> newList{};

	for (auto [profileName, _] : profiles) {
		QString entry = QString::fromStdString(profileName);
		newList.append(entry);
	}

	std::sort(newList.begin(), newList.end(), [&locale](const QString &lhs, const QString &rhs) -> bool {
		int result = QString::localeAwareCompare(locale.toLower(lhs), locale.toLower(rhs));

		return (result < 0);
	});

	sortedProfiles.swap(newList);
}
} // namespace

// MARK: - Main Profile Management Functions

void OBSBasic::SetupNewProfile(const std::string &profileName, bool useWizard)
{
	const OBSProfile &newProfile = CreateProfile(profileName);

	config_set_bool(App()->GetUserConfig(), "Basic", "ConfigOnNewProfile", useWizard);

	ActivateProfile(newProfile, true);

	blog(LOG_INFO, "Created profile '%s' (clean, %s)", newProfile.name.c_str(), newProfile.directoryName.c_str());
	blog(LOG_INFO, "------------------------------------------------");

	if (useWizard) {
		AutoConfig wizard(this);
		wizard.setModal(true);
		wizard.show();
		wizard.exec();
	}
}

void OBSBasic::SetupDuplicateProfile(const std::string &profileName)
{
	const OBSProfile &newProfile = CreateProfile(profileName);
	const OBSProfile &currentProfile = GetCurrentProfile();

	const auto copyOptions = std::filesystem::copy_options::recursive |
				 std::filesystem::copy_options::overwrite_existing;

	try {
		std::filesystem::copy(currentProfile.path, newProfile.path, copyOptions);
	} catch (const std::filesystem::filesystem_error &error) {
		blog(LOG_DEBUG, "%s", error.what());
		throw std::logic_error("Failed to copy files for cloned profile: " + newProfile.name);
	}

	ActivateProfile(newProfile);

	blog(LOG_INFO, "Created profile '%s' (duplicate, %s)", newProfile.name.c_str(),
	     newProfile.directoryName.c_str());
	blog(LOG_INFO, "------------------------------------------------");
}

void OBSBasic::SetupRenameProfile(const std::string &profileName)
{
	const OBSProfile &newProfile = CreateProfile(profileName);
	const OBSProfile currentProfile = GetCurrentProfile();

	const auto copyOptions = std::filesystem::copy_options::recursive |
				 std::filesystem::copy_options::overwrite_existing;

	try {
		std::filesystem::copy(currentProfile.path, newProfile.path, copyOptions);
	} catch (const std::filesystem::filesystem_error &error) {
		blog(LOG_DEBUG, "%s", error.what());
		throw std::logic_error("Failed to copy files for profile: " + currentProfile.name);
	}

	profiles.erase(currentProfile.name);

	ActivateProfile(newProfile);
	RemoveProfile(currentProfile);

	blog(LOG_INFO, "Renamed profile '%s' to '%s' (%s)", currentProfile.name.c_str(), newProfile.name.c_str(),
	     newProfile.directoryName.c_str());
	blog(LOG_INFO, "------------------------------------------------");

	OnEvent(OBS_FRONTEND_EVENT_PROFILE_RENAMED);
}

// MARK: - Profile File Management Functions

const OBSProfile &OBSBasic::CreateProfile(const std::string &profileName)
{
	if (const auto &foundProfile = GetProfileByName(profileName)) {
		throw std::invalid_argument("Profile already exists: " + profileName);
	}

	std::string directoryName;
	if (!GetFileSafeName(profileName.c_str(), directoryName)) {
		throw std::invalid_argument("Failed to create safe directory for new profile: " + profileName);
	}

	std::string profileDirectory;
	profileDirectory.reserve(App()->userProfilesLocation.u8string().size() + OBSProfilePath.size() +
				 directoryName.size());
	profileDirectory.append(App()->userProfilesLocation.u8string()).append(OBSProfilePath).append(directoryName);

	if (!GetClosestUnusedFileName(profileDirectory, nullptr)) {
		throw std::invalid_argument("Failed to get closest directory name for new profile: " + profileName);
	}

	const std::filesystem::path profileDirectoryPath = std::filesystem::u8path(profileDirectory);

	try {
		std::filesystem::create_directory(profileDirectoryPath);
	} catch (const std::filesystem::filesystem_error error) {
		throw std::logic_error("Failed to create directory for new profile: " + profileDirectory);
	}

	const std::filesystem::path profileFile =
		profileDirectoryPath / std::filesystem::u8path(OBSProfileSettingsFile);

	auto [iterator, success] =
		profiles.try_emplace(profileName, OBSProfile{profileName, profileDirectoryPath.filename().u8string(),
							     profileDirectoryPath, profileFile});

	OnEvent(OBS_FRONTEND_EVENT_PROFILE_LIST_CHANGED);

	return iterator->second;
}

void OBSBasic::RemoveProfile(OBSProfile profile)
{
	try {
		std::filesystem::remove_all(profile.path);
	} catch (const std::filesystem::filesystem_error &error) {
		blog(LOG_DEBUG, "%s", error.what());
		throw std::logic_error("Failed to remove profile directory: " + profile.directoryName);
	}

	blog(LOG_INFO, "------------------------------------------------");
	blog(LOG_INFO, "Removed profile '%s' (%s)", profile.name.c_str(), profile.directoryName.c_str());
	blog(LOG_INFO, "------------------------------------------------");
}

// MARK: - Profile UI Handling Functions

bool OBSBasic::CreateNewProfile(const QString &name)
{
	try {
		SetupNewProfile(name.toStdString());
		return true;
	} catch (const std::invalid_argument &error) {
		blog(LOG_ERROR, "%s", error.what());
		return false;
	} catch (const std::logic_error &error) {
		blog(LOG_ERROR, "%s", error.what());
		return false;
	}
}

bool OBSBasic::CreateDuplicateProfile(const QString &name)
{
	try {
		SetupDuplicateProfile(name.toStdString());
		return true;
	} catch (const std::invalid_argument &error) {
		blog(LOG_ERROR, "%s", error.what());
		return false;
	} catch (const std::logic_error &error) {
		blog(LOG_ERROR, "%s", error.what());
		return false;
	}
}

void OBSBasic::DeleteProfile(const QString &name)
{
	const std::string_view currentProfileName{config_get_string(App()->GetUserConfig(), "Basic", "Profile")};

	if (currentProfileName == name.toStdString()) {
		on_actionRemoveProfile_triggered();
		return;
	}

	auto foundProfile = GetProfileByName(name.toStdString());

	if (!foundProfile) {
		blog(LOG_ERROR, "Invalid profile name: %s", QT_TO_UTF8(name));
		return;
	}

	RemoveProfile(foundProfile.value());
	profiles.erase(name.toStdString());

	RefreshProfiles();

	config_save_safe(App()->GetUserConfig(), "tmp", nullptr);

	OnEvent(OBS_FRONTEND_EVENT_PROFILE_LIST_CHANGED);
}

void OBSBasic::ChangeProfile()
{
	QAction *action = reinterpret_cast<QAction *>(sender());

	if (!action) {
		return;
	}

	const std::string_view currentProfileName{config_get_string(App()->GetUserConfig(), "Basic", "Profile")};
	const QVariant qProfileName = action->property("profile_name");
	const std::string selectedProfileName{qProfileName.toString().toStdString()};

	if (currentProfileName == selectedProfileName) {
		action->setChecked(true);
		return;
	}

	const std::optional<OBSProfile> foundProfile = GetProfileByName(selectedProfileName);

	if (!foundProfile) {
		const std::string errorMessage{"Selected profile not found: "};

		throw std::invalid_argument(errorMessage + selectedProfileName.data());
	}

	const OBSProfile &selectedProfile = foundProfile.value();

	OnEvent(OBS_FRONTEND_EVENT_PROFILE_CHANGING);

	ActivateProfile(selectedProfile, true);

	blog(LOG_INFO, "Switched to profile '%s' (%s)", selectedProfile.name.c_str(),
	     selectedProfile.directoryName.c_str());
	blog(LOG_INFO, "------------------------------------------------");
}

void OBSBasic::RefreshProfiles(bool refreshCache)
{
	std::string_view currentProfileName{config_get_string(App()->GetUserConfig(), "Basic", "Profile")};

	QList<QAction *> menuActions = ui->profileMenu->actions();

	for (auto &action : menuActions) {
		QVariant variant = action->property("file_name");

		if (variant.typeName() != nullptr) {
			delete action;
		}
	}

	if (refreshCache) {
		RefreshProfileCache();
	}
	updateSortedProfiles(profiles);

	size_t numAddedProfiles = 0;
	for (auto &name : sortedProfiles) {
		const std::string profileName = name.toStdString();
		try {
			const OBSProfile &profile = profiles.at(profileName);
			const QString qProfileName = QString().fromStdString(profileName);

			QAction *action = new QAction(qProfileName, this);
			action->setProperty("profile_name", qProfileName);
			action->setProperty("file_name", QString().fromStdString(profile.directoryName));
			connect(action, &QAction::triggered, this, &OBSBasic::ChangeProfile);
			action->setCheckable(true);
			action->setChecked(profileName == currentProfileName);

			ui->profileMenu->addAction(action);

			numAddedProfiles += 1;
		} catch (const std::out_of_range &error) {
			blog(LOG_ERROR, "No profile with name %s found in profile cache.\n%s", profileName.c_str(),
			     error.what());
		}
	}

	ui->actionRemoveProfile->setEnabled(numAddedProfiles > 1);
}

// MARK: - Profile Cache Functions

/// Refreshes profile cache data with profile state found on local file system.
void OBSBasic::RefreshProfileCache()
{
	std::map<std::string, OBSProfile> foundProfiles{};

	const std::filesystem::path profilesPath =
		App()->userProfilesLocation / std::filesystem::u8path(OBSProfilePath.substr(1));
	const std::filesystem::path profileSettingsFile = std::filesystem::u8path(OBSProfileSettingsFile);

	if (!std::filesystem::exists(profilesPath)) {
		blog(LOG_WARNING, "Failed to get profiles config path");
		return;
	}

	for (const auto &entry : std::filesystem::directory_iterator(profilesPath)) {
		if (!entry.is_directory()) {
			continue;
		}

		const auto profileCandidate = entry.path() / profileSettingsFile;

		ConfigFile config;

		if (config.Open(profileCandidate.u8string().c_str(), CONFIG_OPEN_EXISTING) != CONFIG_SUCCESS) {
			continue;
		}

		std::string candidateName;
		const char *configName = config_get_string(config, "General", "Name");

		if (configName) {
			candidateName = configName;
		} else {
			candidateName = entry.path().filename().u8string();
		}

		foundProfiles.try_emplace(candidateName, OBSProfile{candidateName, entry.path().filename().u8string(),
								    entry.path(), profileCandidate});
	}

	profiles.swap(foundProfiles);
}

const OBSProfile &OBSBasic::GetCurrentProfile() const
{
	std::string currentProfileName{config_get_string(App()->GetUserConfig(), "Basic", "Profile")};

	if (currentProfileName.empty()) {
		throw std::invalid_argument("No valid profile name in configuration Basic->Profile");
	}

	const auto &foundProfile = profiles.find(currentProfileName);

	if (foundProfile != profiles.end()) {
		return foundProfile->second;
	} else {
		throw std::invalid_argument("Profile not found in profile list: " + currentProfileName);
	}
}

std::optional<OBSProfile> OBSBasic::GetProfileByName(const std::string &profileName) const
{
	auto foundProfile = profiles.find(profileName);

	if (foundProfile == profiles.end()) {
		return {};
	} else {
		return foundProfile->second;
	}
}

std::optional<OBSProfile> OBSBasic::GetProfileByDirectoryName(const std::string &directoryName) const
{
	for (auto &[iterator, profile] : profiles) {
		if (profile.directoryName == directoryName) {
			return profile;
		}
	}

	return {};
}

// MARK: - Qt Slot Functions

void OBSBasic::on_actionNewProfile_triggered()
{
	bool useProfileWizard = config_get_bool(App()->GetUserConfig(), "Basic", "ConfigOnNewProfile");

	const OBSPromptCallback profilePromptCallback = [this](const OBSPromptResult &result) {
		if (GetProfileByName(result.promptValue)) {
			return false;
		}

		return true;
	};

	const OBSPromptRequest request{Str("AddProfile.Title"),          Str("AddProfile.Text"), "", true,
				       Str("AddProfile.WizardCheckbox"), useProfileWizard};

	OBSPromptResult result = PromptForName(request, profilePromptCallback);

	if (!result.success) {
		return;
	}

	try {
		SetupNewProfile(result.promptValue, result.optionValue);
	} catch (const std::invalid_argument &error) {
		blog(LOG_ERROR, "%s", error.what());
	} catch (const std::logic_error &error) {
		blog(LOG_ERROR, "%s", error.what());
	}
}

void OBSBasic::on_actionDupProfile_triggered()
{
	const OBSPromptCallback profilePromptCallback = [this](const OBSPromptResult &result) {
		if (GetProfileByName(result.promptValue)) {
			return false;
		}

		return true;
	};

	const OBSPromptRequest request{Str("AddProfile.Title"), Str("AddProfile.Text")};

	OBSPromptResult result = PromptForName(request, profilePromptCallback);

	if (!result.success) {
		return;
	}

	try {
		SetupDuplicateProfile(result.promptValue);
	} catch (const std::invalid_argument &error) {
		blog(LOG_ERROR, "%s", error.what());
	} catch (const std::logic_error &error) {
		blog(LOG_ERROR, "%s", error.what());
	}
}

void OBSBasic::on_actionRenameProfile_triggered()
{
	const std::string currentProfileName = config_get_string(App()->GetUserConfig(), "Basic", "Profile");

	const OBSPromptCallback profilePromptCallback = [this](const OBSPromptResult &result) {
		if (GetProfileByName(result.promptValue)) {
			return false;
		}

		return true;
	};

	const OBSPromptRequest request{Str("RenameProfile.Title"), Str("AddProfile.Text"), currentProfileName};

	OBSPromptResult result = PromptForName(request, profilePromptCallback);

	if (!result.success) {
		return;
	}

	try {
		SetupRenameProfile(result.promptValue);
	} catch (const std::invalid_argument &error) {
		blog(LOG_ERROR, "%s", error.what());
	} catch (const std::logic_error &error) {
		blog(LOG_ERROR, "%s", error.what());
	}
}

void OBSBasic::on_actionRemoveProfile_triggered(bool skipConfirmation)
{
	if (profiles.size() < 2) {
		return;
	}

	OBSProfile currentProfile;

	try {
		currentProfile = GetCurrentProfile();

		if (!skipConfirmation) {
			const QString confirmationText =
				QTStr("ConfirmRemove.Text").arg(QString::fromStdString(currentProfile.name));

			const QMessageBox::StandardButton button =
				OBSMessageBox::question(this, QTStr("ConfirmRemove.Title"), confirmationText);

			if (button == QMessageBox::No) {
				return;
			}
		}

		OnEvent(OBS_FRONTEND_EVENT_PROFILE_CHANGING);

		profiles.erase(currentProfile.name);
	} catch (const std::invalid_argument &error) {
		blog(LOG_ERROR, "%s", error.what());
	} catch (const std::logic_error &error) {
		blog(LOG_ERROR, "%s", error.what());
	}

	const OBSProfile &newProfile = profiles.begin()->second;

	ActivateProfile(newProfile, true);
	RemoveProfile(currentProfile);

#ifdef YOUTUBE_ENABLED
	if (YouTubeAppDock::IsYTServiceSelected() && !youtubeAppDock)
		NewYouTubeAppDock();
#endif

	blog(LOG_INFO, "Switched to profile '%s' (%s)", newProfile.name.c_str(), newProfile.directoryName.c_str());
	blog(LOG_INFO, "------------------------------------------------");
}

void OBSBasic::on_actionImportProfile_triggered()
{
	const QString home = QDir::homePath();

	const QString sourceDirectory = SelectDirectory(this, QTStr("Basic.MainMenu.Profile.Import"), home);

	if (!sourceDirectory.isEmpty() && !sourceDirectory.isNull()) {
		const std::filesystem::path sourcePath = std::filesystem::u8path(sourceDirectory.toStdString());
		const std::string directoryName = sourcePath.filename().string();

		if (auto profile = GetProfileByDirectoryName(directoryName)) {
			OBSMessageBox::warning(this, QTStr("Basic.MainMenu.Profile.Import"),
					       QTStr("Basic.MainMenu.Profile.Exists"));
			return;
		}

		std::string destinationPathString;
		destinationPathString.reserve(App()->userProfilesLocation.u8string().size() + OBSProfilePath.size() +
					      directoryName.size());
		destinationPathString.append(App()->userProfilesLocation.u8string())
			.append(OBSProfilePath)
			.append(directoryName);

		const std::filesystem::path destinationPath = std::filesystem::u8path(destinationPathString);

		try {
			std::filesystem::create_directory(destinationPath);
		} catch (const std::filesystem::filesystem_error &error) {
			blog(LOG_WARNING, "Failed to create profile directory '%s':\n%s", directoryName.c_str(),
			     error.what());
			return;
		}

		const std::array<std::pair<std::string, bool>, 4> profileFiles{{
			{"basic.ini", true},
			{"service.json", false},
			{"streamEncoder.json", false},
			{"recordEncoder.json", false},
		}};

		for (auto &[file, isMandatory] : profileFiles) {
			const std::filesystem::path sourceFile = sourcePath / std::filesystem::u8path(file);

			if (!std::filesystem::exists(sourceFile)) {
				if (isMandatory) {
					blog(LOG_ERROR,
					     "Failed to import profile from directory '%s' - necessary file '%s' not found",
					     directoryName.c_str(), file.c_str());
					return;
				}
				continue;
			}

			const std::filesystem::path destinationFile = destinationPath / std::filesystem::u8path(file);

			try {
				std::filesystem::copy(sourceFile, destinationFile);
			} catch (const std::filesystem::filesystem_error &error) {
				blog(LOG_WARNING, "Failed to copy import file '%s' for profile '%s':\n%s", file.c_str(),
				     directoryName.c_str(), error.what());
				return;
			}
		}

		RefreshProfiles(true);
	}
}

void OBSBasic::on_actionExportProfile_triggered()
{
	const OBSProfile &currentProfile = GetCurrentProfile();

	const QString home = QDir::homePath();

	const QString destinationDirectory = SelectDirectory(this, QTStr("Basic.MainMenu.Profile.Export"), home);

	const std::array<std::string, 4> profileFiles{"basic.ini", "service.json", "streamEncoder.json",
						      "recordEncoder.json"};

	if (!destinationDirectory.isEmpty() && !destinationDirectory.isNull()) {
		const std::filesystem::path sourcePath = currentProfile.path;
		const std::filesystem::path destinationPath =
			std::filesystem::u8path(destinationDirectory.toStdString()) /
			std::filesystem::u8path(currentProfile.directoryName);

		if (!std::filesystem::exists(destinationPath)) {
			std::filesystem::create_directory(destinationPath);
		}

		std::filesystem::copy_options copyOptions = std::filesystem::copy_options::overwrite_existing;

		for (auto &file : profileFiles) {
			const std::filesystem::path sourceFile = sourcePath / std::filesystem::u8path(file);

			if (!std::filesystem::exists(sourceFile)) {
				continue;
			}

			const std::filesystem::path destinationFile = destinationPath / std::filesystem::u8path(file);

			try {
				std::filesystem::copy(sourceFile, destinationFile, copyOptions);
			} catch (const std::filesystem::filesystem_error &error) {
				blog(LOG_WARNING, "Failed to copy export file '%s' for profile '%s'\n%s", file.c_str(),
				     currentProfile.name.c_str(), error.what());
				return;
			}
		}
	}
}

// MARK: - Profile Management Helper Functions

void OBSBasic::ActivateProfile(const OBSProfile &profile, bool reset)
{
	ConfigFile config;
	if (config.Open(profile.profileFile.u8string().c_str(), CONFIG_OPEN_ALWAYS) != CONFIG_SUCCESS) {
		throw std::logic_error("failed to open configuration file of new profile: " +
				       profile.profileFile.string());
	}

	config_set_string(config, "General", "Name", profile.name.c_str());
	config.SaveSafe("tmp");

	std::vector<std::string> restartRequirements;

	if (activeConfiguration) {
		Auth::Save();

		if (reset) {
			auth.reset();
			DestroyPanelCookieManager();
#ifdef YOUTUBE_ENABLED
			if (youtubeAppDock) {
				DeleteYouTubeAppDock();
			}
#endif
		}
		restartRequirements = GetRestartRequirements(config);

		activeConfiguration.SaveSafe("tmp");
	}

	activeConfiguration.Swap(config);

	config_set_string(App()->GetUserConfig(), "Basic", "Profile", profile.name.c_str());
	config_set_string(App()->GetUserConfig(), "Basic", "ProfileDir", profile.directoryName.c_str());

	config_save_safe(App()->GetUserConfig(), "tmp", nullptr);

	InitBasicConfigDefaults();

	if (reset) {
		UpdateProfileEncoders();
		ResetProfileData();
	}

	RefreshProfiles();

	UpdateTitleBar();
	UpdateVolumeControlsDecayRate();

	Auth::Load();
#ifdef YOUTUBE_ENABLED
	if (YouTubeAppDock::IsYTServiceSelected() && !youtubeAppDock)
		NewYouTubeAppDock();
#endif

	OnEvent(OBS_FRONTEND_EVENT_PROFILE_CHANGED);

	if (!restartRequirements.empty()) {
		std::string requirements = std::accumulate(
			std::next(restartRequirements.begin()), restartRequirements.end(), restartRequirements[0],
			[](std::string a, std::string b) { return std::move(a) + "\n" + b; });

		QMessageBox::StandardButton button = OBSMessageBox::question(
			this, QTStr("Restart"), QTStr("LoadProfileNeedsRestart").arg(requirements.c_str()));

		if (button == QMessageBox::Yes) {
			restart = true;
			close();
		}
	}
}

void OBSBasic::UpdateProfileEncoders()
{
	InitBasicConfigDefaults2();
	CheckForSimpleModeX264Fallback();
}

void OBSBasic::ResetProfileData()
{
	ResetVideo();
	service = nullptr;
	InitService();
	ResetOutputs();
	ClearHotkeys();
	CreateHotkeys();

	/* load audio monitoring */
	if (obs_audio_monitoring_available()) {
		const char *device_name = config_get_string(activeConfiguration, "Audio", "MonitoringDeviceName");
		const char *device_id = config_get_string(activeConfiguration, "Audio", "MonitoringDeviceId");

		obs_set_audio_monitoring_device(device_name, device_id);

		blog(LOG_INFO, "Audio monitoring device:\n\tname: %s\n\tid: %s", device_name, device_id);
	}
}

std::vector<std::string> OBSBasic::GetRestartRequirements(const ConfigFile &config) const
{
	std::vector<std::string> result;

	const char *oldSpeakers = config_get_string(activeConfiguration, "Audio", "ChannelSetup");
	const char *newSpeakers = config_get_string(config, "Audio", "ChannelSetup");

	uint64_t oldSampleRate = config_get_uint(activeConfiguration, "Audio", "SampleRate");
	uint64_t newSampleRate = config_get_uint(config, "Audio", "SampleRate");

	if (oldSpeakers != NULL && newSpeakers != NULL) {
		if (std::string_view{oldSpeakers} != std::string_view{newSpeakers}) {
			result.emplace_back(Str("Basic.Settings.Audio.Channels"));
		}
	}

	if (oldSampleRate != 0 && newSampleRate != 0) {
		if (oldSampleRate != newSampleRate) {
			result.emplace_back(Str("Basic.Settings.Audio.SampleRate"));
		}
	}

	return result;
}

void OBSBasic::CheckForSimpleModeX264Fallback()
{
	const char *curStreamEncoder = config_get_string(activeConfiguration, "SimpleOutput", "StreamEncoder");
	const char *curRecEncoder = config_get_string(activeConfiguration, "SimpleOutput", "RecEncoder");
	bool qsv_supported = false;
	bool qsv_av1_supported = false;
	bool amd_supported = false;
	bool nve_supported = false;
#ifdef ENABLE_HEVC
	bool amd_hevc_supported = false;
	bool nve_hevc_supported = false;
	bool apple_hevc_supported = false;
#endif
	bool amd_av1_supported = false;
	bool apple_supported = false;
	bool changed = false;
	size_t idx = 0;
	const char *id;

	while (obs_enum_encoder_types(idx++, &id)) {
		if (strcmp(id, "h264_texture_amf") == 0)
			amd_supported = true;
		else if (strcmp(id, "obs_qsv11") == 0)
			qsv_supported = true;
		else if (strcmp(id, "obs_qsv11_av1") == 0)
			qsv_av1_supported = true;
		else if (strcmp(id, "ffmpeg_nvenc") == 0)
			nve_supported = true;
#ifdef ENABLE_HEVC
		else if (strcmp(id, "h265_texture_amf") == 0)
			amd_hevc_supported = true;
		else if (strcmp(id, "ffmpeg_hevc_nvenc") == 0)
			nve_hevc_supported = true;
#endif
		else if (strcmp(id, "av1_texture_amf") == 0)
			amd_av1_supported = true;
		else if (strcmp(id, "com.apple.videotoolbox.videoencoder.ave.avc") == 0)
			apple_supported = true;
#ifdef ENABLE_HEVC
		else if (strcmp(id, "com.apple.videotoolbox.videoencoder.ave.hevc") == 0)
			apple_hevc_supported = true;
#endif
	}

	auto CheckEncoder = [&](const char *&name) {
		if (strcmp(name, SIMPLE_ENCODER_QSV) == 0) {
			if (!qsv_supported) {
				changed = true;
				name = SIMPLE_ENCODER_X264;
				return false;
			}
		} else if (strcmp(name, SIMPLE_ENCODER_QSV_AV1) == 0) {
			if (!qsv_av1_supported) {
				changed = true;
				name = SIMPLE_ENCODER_X264;
				return false;
			}
		} else if (strcmp(name, SIMPLE_ENCODER_NVENC) == 0) {
			if (!nve_supported) {
				changed = true;
				name = SIMPLE_ENCODER_X264;
				return false;
			}
		} else if (strcmp(name, SIMPLE_ENCODER_NVENC_AV1) == 0) {
			if (!nve_supported) {
				changed = true;
				name = SIMPLE_ENCODER_X264;
				return false;
			}
#ifdef ENABLE_HEVC
		} else if (strcmp(name, SIMPLE_ENCODER_AMD_HEVC) == 0) {
			if (!amd_hevc_supported) {
				changed = true;
				name = SIMPLE_ENCODER_X264;
				return false;
			}
		} else if (strcmp(name, SIMPLE_ENCODER_NVENC_HEVC) == 0) {
			if (!nve_hevc_supported) {
				changed = true;
				name = SIMPLE_ENCODER_X264;
				return false;
			}
#endif
		} else if (strcmp(name, SIMPLE_ENCODER_AMD) == 0) {
			if (!amd_supported) {
				changed = true;
				name = SIMPLE_ENCODER_X264;
				return false;
			}
		} else if (strcmp(name, SIMPLE_ENCODER_AMD_AV1) == 0) {
			if (!amd_av1_supported) {
				changed = true;
				name = SIMPLE_ENCODER_X264;
				return false;
			}
		} else if (strcmp(name, SIMPLE_ENCODER_APPLE_H264) == 0) {
			if (!apple_supported) {
				changed = true;
				name = SIMPLE_ENCODER_X264;
				return false;
			}
#ifdef ENABLE_HEVC
		} else if (strcmp(name, SIMPLE_ENCODER_APPLE_HEVC) == 0) {
			if (!apple_hevc_supported) {
				changed = true;
				name = SIMPLE_ENCODER_X264;
				return false;
			}
#endif
		}

		return true;
	};

	if (!CheckEncoder(curStreamEncoder))
		config_set_string(activeConfiguration, "SimpleOutput", "StreamEncoder", curStreamEncoder);
	if (!CheckEncoder(curRecEncoder))
		config_set_string(activeConfiguration, "SimpleOutput", "RecEncoder", curRecEncoder);
	if (changed) {
		activeConfiguration.SaveSafe("tmp");
	}
}
