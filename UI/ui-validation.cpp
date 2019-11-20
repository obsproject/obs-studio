#include "ui-validation.hpp"

#include <obs.hpp>
#include <QString>
#include <QMessageBox>
#include <QPushButton>

#include <obs-app.hpp>

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
}
