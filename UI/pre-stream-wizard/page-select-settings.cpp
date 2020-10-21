#include "page-select-settings.hpp"

#include <QSpacerItem>
#include <QPushButton>
#include <QDesktopServices>
#include <QVariant>
#include <QPair>

#include "obs-app.hpp"
#include "setting-selection-row.hpp"

namespace StreamWizard {

SelectionPage::SelectionPage(QWidget *parent) : QWizardPage(parent)
{
	this->setTitle(QTStr("PreLiveWizard.Selection.Title"));
	setupLayout();

	setButtonText(QWizard::WizardButton::NextButton,
		      QTStr("Basic.AutoConfig.ApplySettings"));
	setButtonText(QWizard::WizardButton::CommitButton,
		      QTStr("Basic.AutoConfig.ApplySettings"));
	setCommitPage(true);
}

void SelectionPage::setupLayout()
{
	QVBoxLayout *mainLayout = new QVBoxLayout(this);
	setLayout(mainLayout);

	// Description to user what scroll area contains
	descriptionLabel_ =
		new QLabel(QTStr("PreLiveWizard.Selection.Description"));
	mainLayout->addWidget(descriptionLabel_);

	// Add Scroll area widget
	scrollArea_ = new QScrollArea();
	scrollArea_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
	scrollArea_->setWidgetResizable(true);

	// ScrollArea_ holds a widget:
	scrollBoxWidget_ = new QWidget();
	scrollArea_->setWidget(scrollBoxWidget_);

	// Scroll's Widget requires a layout
	scrollVBoxLayout_ = new QVBoxLayout();

	scrollBoxWidget_->setLayout(scrollVBoxLayout_);
	scrollBoxWidget_->show();

	// Add Scroll Aread to mainlayout
	mainLayout->addWidget(scrollArea_);
}

void SelectionPage::setSettingsMap(QSharedPointer<SettingsMap> settingsMap)
{
	if (settingsMap_ != nullptr) {
		return;
	}
	settingsMap_ = settingsMap;
	SettingsMap *mapInfo = settingsMap_.data();

	if (mapInfo->contains(kSettingsResponseKeys.videoBitrate)) {
		QVariant data =
			mapInfo->value(kSettingsResponseKeys.videoBitrate).first;
		QString bitrateString = QString::number(data.toInt()) + "kbps";
		addRow(QTStr("Basic.Settings.Output.VideoBitrate"),
		       bitrateString, kSettingsResponseKeys.videoBitrate);
	}

	// Uses kSettingsResponseKeys.videoHeight to signal change
	if (mapInfo->contains(kSettingsResponseKeys.videoWidth) &&
	    mapInfo->contains(kSettingsResponseKeys.videoHeight)) {
		int vidW = mapInfo->value(kSettingsResponseKeys.videoWidth)
				   .first.toInt();
		QString videoWidthString = QString::number(vidW);
		int vidH = mapInfo->value(kSettingsResponseKeys.videoHeight)
				   .first.toInt();
		QString videoHeightString = QString::number(vidH);
		QString valueString =
			videoWidthString + "x" + videoHeightString;

		addRow(QTStr("Basic.Settings.Video.ScaledResolution"),
		       valueString, kSettingsResponseKeys.videoHeight);
	}

	if (mapInfo->contains(kSettingsResponseKeys.framerate)) {
		QVariant data =
			mapInfo->value(kSettingsResponseKeys.framerate).first;
		double framerate = data.toDouble();
		QString fpsString = QString().asprintf("%.3f", framerate);
		addRow(QTStr("Basic.Settings.Video.FPS"), fpsString,
		       kSettingsResponseKeys.framerate);
	}

	if (mapInfo->contains(kSettingsResponseKeys.h264Profile)) {
		QVariant data =
			mapInfo->value(kSettingsResponseKeys.h264Profile).first;
		QString profileString = data.toString();
		addRow(QTStr("PreLiveWizard.Output.Mode.CodecProfile"),
		       profileString, kSettingsResponseKeys.h264Profile);
	}

	if (mapInfo->contains(kSettingsResponseKeys.h264Level)) {
		QVariant data =
			mapInfo->value(kSettingsResponseKeys.h264Level).first;
		QString level = data.toString();
		addRow(QTStr("PreLiveWizard.Output.Mode.CodecLevel"), level,
		       kSettingsResponseKeys.h264Level);
	}

	if (mapInfo->contains(kSettingsResponseKeys.gopSizeInFrames)) {
		QVariant data =
			mapInfo->value(kSettingsResponseKeys.gopSizeInFrames)
				.first;
		QString gopFramesString = QString::number(data.toInt());
		addRow(QTStr("Basic.Settings.Output.Adv.FFmpeg.GOPSize"),
		       gopFramesString, kSettingsResponseKeys.gopSizeInFrames);
	}

	if (mapInfo->contains(kSettingsResponseKeys.gopClosed)) {
		QVariant data =
			mapInfo->value(kSettingsResponseKeys.gopClosed).first;
		QString gopClosedString = data.toBool() ? QTStr("Yes")
							: QTStr("No");
		addRow(QTStr("Basic.Settings.Output.Adv.FFmpeg.GOPClosed"),
		       gopClosedString, kSettingsResponseKeys.gopClosed);
	}

	if (mapInfo->contains(kSettingsResponseKeys.gopBFrames)) {
		QVariant data =
			mapInfo->value(kSettingsResponseKeys.gopBFrames).first;
		QString bFramesString = QString::number(data.toInt());
		addRow(QTStr("Basic.Settings.Output.Adv.FFmpeg.BFrames"),
		       bFramesString, kSettingsResponseKeys.gopBFrames);
	}

	if (mapInfo->contains(kSettingsResponseKeys.gopRefFrames)) {
		QVariant data =
			mapInfo->value(kSettingsResponseKeys.gopRefFrames).first;
		QString gopRefFramesCountString = QString::number(data.toInt());
		addRow(QTStr("Basic.Settings.Output.Adv.FFmpeg.ReferenceFrames"),
		       gopRefFramesCountString,
		       kSettingsResponseKeys.gopRefFrames);
	}

	if (mapInfo->contains(kSettingsResponseKeys.streamRateControlMode)) {
		QVariant data = mapInfo->value(kSettingsResponseKeys
						       .streamRateControlMode)
					.first;
		QString rateControlString = data.toString().toUpper();
		addRow(QTStr("PreLiveWizard.Output.Mode.RateControl"),
		       rateControlString,
		       kSettingsResponseKeys.streamRateControlMode);
	}

	if (mapInfo->contains(kSettingsResponseKeys.streamBufferSize)) {
		QVariant data =
			mapInfo->value(kSettingsResponseKeys.streamBufferSize)
				.first;
		QString bufferSizeString = QString::number(data.toInt()) + "Kb";
		addRow(QTStr("PreLiveWizard.Output.Mode.StreamBuffer"),
		       bufferSizeString,
		       kSettingsResponseKeys.streamBufferSize);
	}
}

void SelectionPage::addRow(QString title, QString value, QString mapKey)
{
	SelectionRow *row = new SelectionRow();
	row->setName(title);
	row->setValueLabel(value);
	row->setPropertyKey(mapKey);
	scrollVBoxLayout_->addWidget(row);
	connect(row, &SelectionRow::didChangeSelectedStatus, this,
		&SelectionPage::checkboxRowChanged);
}

void SelectionPage::checkboxRowChanged(QString propertyKey, bool selected)
{
	if (settingsMap_ == nullptr) {
		return;
	}

	SettingsMap *mapInfo = settingsMap_.data();
	if (mapInfo == nullptr || !mapInfo->contains(propertyKey)) {
		return;
	}

	QPair<QVariant, bool> &dataPair = (*mapInfo)[propertyKey];
	dataPair.second = selected;
}

} // namespace StreamWizard
