#include "page-select-settings.hpp"

#include <QSpacerItem>
#include <QPushButton>
#include <QDesktopServices>
#include <QDebug>
#include <QVariant>
#include <QPair>

#include "obs-app.hpp"
#include "setting-selection-row.hpp"

namespace StreamWizard {

SelectionPage::SelectionPage(QWidget *parent) : QWizardPage(parent)
{
	this->setTitle(QTStr("PreLiveWizard.Selection.Title"));
	setupLayout();
}

SelectionPage::~SelectionPage()
{
	// nop
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
	SettingsMap *mapInfo = settingsMap_.get();

	if (mapInfo->contains(SettingsResponseKeys.videoBitrate)) {
		QVariant data =
			mapInfo->value(SettingsResponseKeys.videoBitrate).first;
		QString bitrateString = QString::number(data.toInt()) + "kbps";
		addRow(QTStr("Basic.Settings.Output.VideoBitrate"),
		       bitrateString, SettingsResponseKeys.videoBitrate);
	}

	// Uses SettingsResponseKeys.videoHeight to signal change
	if (mapInfo->contains(SettingsResponseKeys.videoWidth) &&
	    mapInfo->contains(SettingsResponseKeys.videoHeight)) {
		int vidW = mapInfo->value(SettingsResponseKeys.videoWidth)
				   .first.toInt();
		QString videoWidthString = QString::number(vidW);
		int vidH = mapInfo->value(SettingsResponseKeys.videoHeight)
				   .first.toInt();
		QString videoHeightString = QString::number(vidH);
		QString valueString =
			videoWidthString + "x" + videoHeightString;

		addRow(QTStr("Basic.Settings.Video.ScaledResolution"),
		       valueString, SettingsResponseKeys.videoHeight);
	}

	if (mapInfo->contains(SettingsResponseKeys.framerate)) {
		QVariant data =
			mapInfo->value(SettingsResponseKeys.framerate).first;
		double framerate = data.toDouble();
		QString fpsString = QString().sprintf("%.3f", framerate);
		addRow(QTStr("Basic.Settings.Video.FPS"), fpsString,
		       SettingsResponseKeys.framerate);
	}

	if (mapInfo->contains(SettingsResponseKeys.h264Profile)) {
		QVariant data =
			mapInfo->value(SettingsResponseKeys.h264Profile).first;
		QString profileString = data.toString();
		addRow(QTStr("Basic.Settings.Output.Mode.CodecProfile"),
		       profileString, SettingsResponseKeys.h264Profile);
	}

	if (mapInfo->contains(SettingsResponseKeys.h264Level)) {
		QVariant data =
			mapInfo->value(SettingsResponseKeys.h264Level).first;
		QString level = data.toString();
		addRow(QTStr("Basic.Settings.Output.Mode.CodecLevel"), level,
		       SettingsResponseKeys.h264Level);
	}

	if (mapInfo->contains(SettingsResponseKeys.gopSizeInFrames)) {
		QVariant data =
			mapInfo->value(SettingsResponseKeys.gopSizeInFrames)
				.first;
		QString gopFramesString = QString::number(data.toInt());
		addRow(QTStr("Basic.Settings.Output.Adv.FFmpeg.GOPSize"),
		       gopFramesString, SettingsResponseKeys.gopSizeInFrames);
	}

	if (mapInfo->contains(SettingsResponseKeys.gopType)) {
		QVariant data =
			mapInfo->value(SettingsResponseKeys.gopType).first;
		QString gopTypeString = data.toString();
		addRow(QTStr("Basic.Settings.Output.Adv.FFmpeg.GOPType"),
		       gopTypeString, SettingsResponseKeys.gopType);
	}

	if (mapInfo->contains(SettingsResponseKeys.gopClosed)) {
		QVariant data =
			mapInfo->value(SettingsResponseKeys.gopClosed).first;
		QString gopClosedString = data.toBool() ? QTStr("Yes")
							: QTStr("No");
		addRow(QTStr("Basic.Settings.Output.Adv.FFmpeg.GOPClosed"),
		       gopClosedString, SettingsResponseKeys.gopClosed);
	}

	if (mapInfo->contains(SettingsResponseKeys.gopBFrames)) {
		QVariant data =
			mapInfo->value(SettingsResponseKeys.gopBFrames).first;
		QString bFramesString = QString::number(data.toInt());
		addRow(QTStr("Basic.Settings.Output.Adv.FFmpeg.BFrames"),
		       bFramesString, SettingsResponseKeys.gopBFrames);
	}

	if (mapInfo->contains(SettingsResponseKeys.gopRefFrames)) {
		QVariant data =
			mapInfo->value(SettingsResponseKeys.gopRefFrames).first;
		QString gopRefFramesCountString = QString::number(data.toInt());
		addRow(QTStr("Basic.Settings.Output.Adv.FFmpeg.ReferenceFrames"),
		       gopRefFramesCountString,
		       SettingsResponseKeys.gopRefFrames);
	}

	if (mapInfo->contains(SettingsResponseKeys.streamRateControlMode)) {
		QVariant data = mapInfo->value(SettingsResponseKeys
						       .streamRateControlMode)
					.first;
		QString rateControlString = data.toString().toUpper();
		addRow(QTStr("Basic.Settings.Output.Mode.RateControl"),
		       rateControlString,
		       SettingsResponseKeys.streamRateControlMode);
	}

	if (mapInfo->contains(SettingsResponseKeys.streamBufferSize)) {
		QVariant data =
			mapInfo->value(SettingsResponseKeys.streamBufferSize)
				.first;
		QString bufferSizeString =
			QString::number(data.toInt()) + " Kb";
		addRow(QTStr("Basic.Settings.Output.Mode.StreamBuffer"),
		       bufferSizeString, SettingsResponseKeys.streamBufferSize);
	}
}

void SelectionPage::addRow(QString title, QString value, const char *mapKey)
{
	SelectionRow *row = new SelectionRow();
	row->setName(title);
	row->setValueLabel(value);
	row->setPropertyKey(mapKey);
	scrollVBoxLayout_->addWidget(row);
	connect(row, &SelectionRow::didChangeSelectedStatus, this,
		&SelectionPage::checkboxRowChanged);
}

void SelectionPage::checkboxRowChanged(const char *propertyKey, bool selected)
{
	if (settingsMap_ == nullptr) {
		return;
	}

	SettingsMap *mapInfo = settingsMap_.get();

	if (mapInfo == nullptr || !mapInfo->contains(propertyKey)) {
		return;
	}

	QPair dataPair = mapInfo->value(propertyKey);
	dataPair.second = selected;
}

} // namespace StreamWizard
