#include "ui-validation.hpp"

#include <obs.hpp>
#include <QString>
#include <QMessageBox>
#include <QPushButton>

#include <obs-app.hpp>
#include <obs-service.h>

#ifdef _WIN32
#include <windows.h>
#endif
#include "ui-config.h"

#ifdef HTTP_REST_API_ENABLED
#include "obs-http-api.hpp"
#endif

static int CountVideoSources()
{
	int count = 0;
	auto countSources = [](void *param, obs_source_t *source) {
		if (!source)
			return true;

		uint32_t flags = obs_source_get_output_flags(source);
		if ((flags & OBS_SOURCE_VIDEO) != 0)
			(*reinterpret_cast<int *>(param))++;

		return true;
	};

	obs_enum_sources(countSources, &count);
	return count;
}

bool UIValidation::NoSourcesConfirmation(QWidget *parent)
{
	// There are sources, don't need confirmation
	if (CountVideoSources() != 0)
		return true;

	// Ignore no video if no parent is visible to alert on
	if (!parent->isVisible())
		return true;
#ifdef HTTP_REST_API_ENABLED
	// Ignore no video alert for Spoon Radio : Simon Ahn
	// Spoon radio always transmits only audio.
	return true;
#else 
	QString msg = QTStr("NoSources.Text");
	msg += "\n\n";
	msg += QTStr("NoSources.Text.AddSource");

	QMessageBox messageBox(parent);
	messageBox.setWindowTitle(QTStr("NoSources.Title"));
	messageBox.setText(msg);

	QAbstractButton *yesButton =
		messageBox.addButton(QTStr("Yes"), QMessageBox::YesRole);
	messageBox.addButton(QTStr("No"), QMessageBox::NoRole);
	messageBox.setIcon(QMessageBox::Question);
	messageBox.exec();

	if (messageBox.clickedButton() != yesButton)
		return false;
	else
		return true;
#endif
}

StreamSettingsAction
UIValidation::StreamSettingsConfirmation(QWidget *parent, OBSService service)
{
	// Custom services can user API key in URL or user/pass combo.
	// So only check there is a URL
	char const *serviceType = obs_service_get_type(service);
	bool isCustomUrlService = (strcmp(serviceType, "rtmp_custom") == 0);

#ifdef HTTP_REST_API_ENABLED
	char const *tempStreamUrl = obs_service_get_url(service);
	char const *tempStreamKey = obs_service_get_key(service);

	char *streamUrl = NULL;
	char *streamKey = NULL;

#ifdef _WIN32
    FILE *file = fopen("OBS_HTTP_API.DAT", "r");
#else
    char homeDocumentsDir[1024] = {0};
    snprintf(homeDocumentsDir, 1024, "%s/Documents/.OBS_HTTP_API.DAT", getenv("HOME"));
    FILE *file = fopen(homeDocumentsDir, "r");
#endif
	if (file) {
		OBSHttpApiValue streamValues;  
		fread(&streamValues, sizeof(OBSHttpApiValue), 1, file);
		fclose(file);

		OBSDataAutoRelease service_settings = obs_data_create();

		streamUrl = streamValues.streamUrl;
		streamKey = streamValues.streamKey;

		obs_data_set_string(service_settings, "service", "Spoon Radio");
		obs_data_set_string(service_settings, "server", streamUrl);
		obs_data_set_string(service_settings, "key", streamKey);
		obs_service_update(service, service_settings);

		// TEST CODE
		char const *tempStreamUrl1 = obs_service_get_url(service);
		char const *tempStreamKey1 = obs_service_get_key(service);
		blog(LOG_INFO, "===== Connected server & key   ========================");
		blog(LOG_INFO, " Stream Url : %s", tempStreamUrl1);
		blog(LOG_INFO, " Stream Key : %s", tempStreamKey1);

	} else {
		streamUrl = (char *)tempStreamUrl;
		streamKey = (char *)tempStreamKey;
	}
#else
	char const *streamUrl = obs_service_get_url(service);
	char const *streamKey = obs_service_get_key(service);
#endif
	bool hasStreamUrl = (streamUrl != NULL && streamUrl[0] != '\0');
	bool hasStreamKey = ((streamKey != NULL && streamKey[0] != '\0') ||
			     isCustomUrlService);

	if (hasStreamUrl && hasStreamKey)
		return StreamSettingsAction::ContinueStream;

	QString msg;

	if (!hasStreamUrl && !hasStreamKey) {
		msg = QTStr("Basic.Settings.Stream.MissingUrlAndApiKey");
	} else if (!hasStreamKey) {
		msg = QTStr("Basic.Settings.Stream.MissingStreamKey");
	} else {
		msg = QTStr("Basic.Settings.Stream.MissingUrl");
	}

	QMessageBox messageBox(parent);
	messageBox.setWindowTitle(
		QTStr("Basic.Settings.Stream.MissingSettingAlert"));
	messageBox.setText(msg);

	QPushButton *cancel;
	QPushButton *settings;

#ifdef __APPLE__
#define ACCEPT_BUTTON QMessageBox::AcceptRole
#define REJECT_BUTTON QMessageBox::ResetRole
#else
#define ACCEPT_BUTTON QMessageBox::NoRole
#define REJECT_BUTTON QMessageBox::NoRole
#endif
	settings = messageBox.addButton(
		QTStr("Basic.Settings.Stream.StreamSettingsWarning"),
		ACCEPT_BUTTON);
	cancel = messageBox.addButton(QTStr("Cancel"), REJECT_BUTTON);

	messageBox.setDefaultButton(settings);
	messageBox.setEscapeButton(cancel);

	messageBox.setIcon(QMessageBox::Warning);
	messageBox.exec();

	if (messageBox.clickedButton() == settings)
		return StreamSettingsAction::OpenSettings;
	if (messageBox.clickedButton() == cancel)
		return StreamSettingsAction::Cancel;

	return StreamSettingsAction::ContinueStream;
}
