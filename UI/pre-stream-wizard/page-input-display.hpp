#pragma once

#include <QWizardPage>
#include <QSharedPointer>

#include "pre-stream-current-settings.hpp"

extern "C" {

QWizardPage *SettingsInputPage(StreamWizard::EncoderSettingsRequest *settings);

} // extern C
