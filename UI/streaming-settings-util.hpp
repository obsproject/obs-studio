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
	makeEncoderSettingsFromCurrentState(config_t *config,
					    obs_data_t *settings);
};
