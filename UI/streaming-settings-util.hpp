#pragma once

#include <QObject>
#include <QSharedPointer>

#include <obs.hpp>
#include "pre-stream-current-settings.hpp"

#include "obs-app.hpp"

class StreamingSettingsUtility : public QObject {
	Q_OBJECT

public:
	// Uses current settings in OBS
	static QSharedPointer<StreamWizard::EncoderSettingsRequest>
	makeEncoderSettingsFromCurrentState(config_t *config);

	static void applyWizardSettings(
		QSharedPointer<StreamWizard::SettingsMap> newSettings,
		config_t *config);
};
