/*
obs-websocket
Copyright (C) 2016-2021 Stephane Lepin <stephane.lepin@gmail.com>
Copyright (C) 2020-2021 Kyle Manning <tt2468@gmail.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program. If not, see <https://www.gnu.org/licenses/>
*/

#include <QtWidgets/QMessageBox>
#include <QDateTime>
#include <QTime>
#include <obs-module.h>
#include <obs.hpp>

#include "SettingsDialog.h"
#include "../obs-websocket.h"
#include "../Config.h"
#include "../websocketserver/WebSocketServer.h"
#include "../utils/Crypto.h"

QString GetToolTipIconHtml()
{
	bool lightTheme = QApplication::palette().text().color().redF() < 0.5;
	QString iconFile = lightTheme ? ":toolTip/images/help.svg" : ":toolTip/images/help_light.svg";
	QString iconTemplate = "<html> <img src='%1' style=' vertical-align: bottom; ' /></html>";
	return iconTemplate.arg(iconFile);
}

SettingsDialog::SettingsDialog(QWidget *parent)
	: QDialog(parent, Qt::Dialog),
	  ui(new Ui::SettingsDialog),
	  connectInfo(new ConnectInfo),
	  sessionTableTimer(new QTimer),
	  passwordManuallyEdited(false)
{
	ui->setupUi(this);
	ui->websocketSessionTable->horizontalHeader()->resizeSection(3, 100); // Resize Session Table column widths
	ui->websocketSessionTable->horizontalHeader()->resizeSection(4, 100);

	// Remove the ? button on dialogs on Windows
	setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

	// Set the appropriate tooltip icon for the theme
	ui->enableDebugLoggingToolTipLabel->setText(GetToolTipIconHtml());

	connect(sessionTableTimer, &QTimer::timeout, this, &SettingsDialog::FillSessionTable);
	connect(ui->buttonBox, &QDialogButtonBox::clicked, this, &SettingsDialog::DialogButtonClicked);
#if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0)
	connect(ui->enableAuthenticationCheckBox, &QCheckBox::checkStateChanged, this,
		&SettingsDialog::EnableAuthenticationCheckBoxChanged);
#else
	connect(ui->enableAuthenticationCheckBox, &QCheckBox::stateChanged, this,
		&SettingsDialog::EnableAuthenticationCheckBoxChanged);
#endif
	connect(ui->generatePasswordButton, &QPushButton::clicked, this, &SettingsDialog::GeneratePasswordButtonClicked);
	connect(ui->showConnectInfoButton, &QPushButton::clicked, this, &SettingsDialog::ShowConnectInfoButtonClicked);
	connect(ui->serverPasswordLineEdit, &QLineEdit::textEdited, this, &SettingsDialog::PasswordEdited);
}

SettingsDialog::~SettingsDialog()
{
	delete ui;
	delete connectInfo;
	delete sessionTableTimer;
}

void SettingsDialog::showEvent(QShowEvent *)
{
	auto conf = GetConfig();
	if (!conf) {
		blog(LOG_ERROR, "[SettingsDialog::showEvent] Unable to retreive config!");
		return;
	}

	if (conf->PortOverridden)
		ui->serverPortSpinBox->setEnabled(false);

	if (conf->PasswordOverridden) {
		ui->enableAuthenticationCheckBox->setEnabled(false);
		ui->serverPasswordLineEdit->setEnabled(false);
		ui->generatePasswordButton->setEnabled(false);
	}

	passwordManuallyEdited = false;

	RefreshData();

	sessionTableTimer->start(1000);
}

void SettingsDialog::hideEvent(QHideEvent *)
{
	if (sessionTableTimer->isActive())
		sessionTableTimer->stop();

	connectInfo->hide();
}

void SettingsDialog::ToggleShowHide()
{
	if (!isVisible())
		setVisible(true);
	else
		setVisible(false);
}

void SettingsDialog::RefreshData()
{
	auto conf = GetConfig();
	if (!conf) {
		blog(LOG_ERROR, "[SettingsDialog::RefreshData] Unable to retreive config!");
		return;
	}

	ui->enableWebSocketServerCheckBox->setChecked(conf->ServerEnabled);
	ui->enableSystemTrayAlertsCheckBox->setChecked(conf->AlertsEnabled);
	ui->enableDebugLoggingCheckBox->setChecked(conf->DebugEnabled);
	ui->serverPortSpinBox->setValue(conf->ServerPort);
	ui->enableAuthenticationCheckBox->setChecked(conf->AuthRequired);
	ui->serverPasswordLineEdit->setText(QString::fromStdString(conf->ServerPassword));

	ui->serverPasswordLineEdit->setEnabled(conf->AuthRequired);
	ui->generatePasswordButton->setEnabled(conf->AuthRequired);

	FillSessionTable();
}

void SettingsDialog::DialogButtonClicked(QAbstractButton *button)
{
	if (button == ui->buttonBox->button(QDialogButtonBox::Ok)) {
		SaveFormData();
	} else if (button == ui->buttonBox->button(QDialogButtonBox::Apply)) {
		SaveFormData();
	}
}

void SettingsDialog::SaveFormData()
{
	auto conf = GetConfig();
	if (!conf) {
		blog(LOG_ERROR, "[SettingsDialog::SaveFormData] Unable to retreive config!");
		return;
	}

	if (ui->serverPasswordLineEdit->text().length() < 6) {
		QMessageBox msgBox;
		msgBox.setWindowTitle(obs_module_text("OBSWebSocket.Settings.Save.PasswordInvalidErrorTitle"));
		msgBox.setText(obs_module_text("OBSWebSocket.Settings.Save.PasswordInvalidErrorMessage"));
		msgBox.setStandardButtons(QMessageBox::Ok);
		msgBox.exec();
		return;
	}

	// Show a confirmation box to the user if they attempt to provide their own password
	if (passwordManuallyEdited && (conf->ServerPassword != ui->serverPasswordLineEdit->text().toStdString())) {
		QMessageBox msgBox;
		msgBox.setWindowTitle(obs_module_text("OBSWebSocket.Settings.Save.UserPasswordWarningTitle"));
		msgBox.setText(obs_module_text("OBSWebSocket.Settings.Save.UserPasswordWarningMessage"));
		msgBox.setInformativeText(obs_module_text("OBSWebSocket.Settings.Save.UserPasswordWarningInfoText"));
		msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
		msgBox.setDefaultButton(QMessageBox::No);
		int ret = msgBox.exec();

		switch (ret) {
		case QMessageBox::Yes:
			break;
		case QMessageBox::No:
		default:
			ui->serverPasswordLineEdit->setText(QString::fromStdString(conf->ServerPassword));
			return;
		}
	}

	bool needsRestart = (conf->ServerEnabled != ui->enableWebSocketServerCheckBox->isChecked()) ||
			    (conf->ServerPort != ui->serverPortSpinBox->value()) ||
			    (ui->enableAuthenticationCheckBox->isChecked() &&
			     conf->ServerPassword != ui->serverPasswordLineEdit->text().toStdString());

	conf->ServerEnabled = ui->enableWebSocketServerCheckBox->isChecked();
	conf->AlertsEnabled = ui->enableSystemTrayAlertsCheckBox->isChecked();
	conf->DebugEnabled = ui->enableDebugLoggingCheckBox->isChecked();
	conf->ServerPort = ui->serverPortSpinBox->value();
	conf->AuthRequired = ui->enableAuthenticationCheckBox->isChecked();
	conf->ServerPassword = ui->serverPasswordLineEdit->text().toStdString();

	conf->Save();

	RefreshData();
	connectInfo->RefreshData();

	if (needsRestart) {
		blog(LOG_INFO, "[SettingsDialog::SaveFormData] A setting was changed which requires a server restart.");
		auto server = GetWebSocketServer();
		server->Stop();
		if (conf->ServerEnabled) {
			server->Start();
		}
	}
}

void SettingsDialog::FillSessionTable()
{
	auto webSocketServer = GetWebSocketServer();
	if (!webSocketServer) {
		blog(LOG_ERROR, "[SettingsDialog::FillSessionTable] Unable to fetch websocket server instance!");
		return;
	}

	auto webSocketSessions = webSocketServer->GetWebSocketSessions();
	size_t rowCount = webSocketSessions.size();

	// Manually setting the pixmap size *might* break with highdpi. Not sure though
	QIcon checkIcon = style()->standardIcon(QStyle::SP_DialogOkButton);
	QPixmap checkIconPixmap = checkIcon.pixmap(QSize(25, 25));
	QIcon crossIcon = style()->standardIcon(QStyle::SP_DialogCancelButton);
	QPixmap crossIconPixmap = crossIcon.pixmap(QSize(25, 25));

	// Todo: Make a util for translations so that we don't need to import a bunch of obs libraries in order to use them.
	QString kickButtonText = obs_module_text("OBSWebSocket.SessionTable.KickButtonText");

	ui->websocketSessionTable->setRowCount(rowCount);
	size_t i = 0;
	for (auto session : webSocketSessions) {
		QTableWidgetItem *addressItem = new QTableWidgetItem(QString::fromStdString(session.remoteAddress));
		ui->websocketSessionTable->setItem(i, 0, addressItem);

		uint64_t sessionDuration = QDateTime::currentSecsSinceEpoch() - session.connectedAt;
		QTableWidgetItem *durationItem = new QTableWidgetItem(QTime(0, 0, sessionDuration).toString("hh:mm:ss"));
		ui->websocketSessionTable->setItem(i, 1, durationItem);

		QTableWidgetItem *statsItem =
			new QTableWidgetItem(QString("%1/%2").arg(session.incomingMessages).arg(session.outgoingMessages));
		ui->websocketSessionTable->setItem(i, 2, statsItem);

		QLabel *identifiedLabel = new QLabel();
		identifiedLabel->setAlignment(Qt::AlignCenter);
		if (session.isIdentified) {
			identifiedLabel->setPixmap(checkIconPixmap);
		} else {
			identifiedLabel->setPixmap(crossIconPixmap);
		}
		ui->websocketSessionTable->setCellWidget(i, 3, identifiedLabel);

		QPushButton *invalidateButton = new QPushButton(kickButtonText, this);
		QWidget *invalidateButtonWidget = new QWidget();
		QHBoxLayout *invalidateButtonLayout = new QHBoxLayout(invalidateButtonWidget);
		invalidateButtonLayout->addWidget(invalidateButton);
		invalidateButtonLayout->setAlignment(Qt::AlignCenter);
		invalidateButtonLayout->setContentsMargins(0, 0, 0, 0);
		invalidateButtonWidget->setLayout(invalidateButtonLayout);
		ui->websocketSessionTable->setCellWidget(i, 4, invalidateButtonWidget);
		connect(invalidateButton, &QPushButton::clicked, [=]() { webSocketServer->InvalidateSession(session.hdl); });

		i++;
	}
}

void SettingsDialog::EnableAuthenticationCheckBoxChanged()
{
	if (ui->enableAuthenticationCheckBox->isChecked()) {
		ui->serverPasswordLineEdit->setEnabled(true);
		ui->generatePasswordButton->setEnabled(true);
	} else {
		ui->serverPasswordLineEdit->setEnabled(false);
		ui->generatePasswordButton->setEnabled(false);
	}
}

void SettingsDialog::GeneratePasswordButtonClicked()
{
	QString newPassword = QString::fromStdString(Utils::Crypto::GeneratePassword());
	ui->serverPasswordLineEdit->setText(newPassword);
	ui->serverPasswordLineEdit->selectAll();
	passwordManuallyEdited = false;
}

void SettingsDialog::ShowConnectInfoButtonClicked()
{
	if (obs_video_active()) {
		QMessageBox msgBox;
		msgBox.setWindowTitle(obs_module_text("OBSWebSocket.Settings.ShowConnectInfoWarningTitle"));
		msgBox.setText(obs_module_text("OBSWebSocket.Settings.ShowConnectInfoWarningMessage"));
		msgBox.setInformativeText(obs_module_text("OBSWebSocket.Settings.ShowConnectInfoWarningInfoText"));
		msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
		msgBox.setDefaultButton(QMessageBox::No);
		int ret = msgBox.exec();

		switch (ret) {
		case QMessageBox::Yes:
			break;
		case QMessageBox::No:
		default:
			return;
		}
	}

	connectInfo->show();
	connectInfo->activateWindow();
	connectInfo->raise();
	connectInfo->setFocus();
}

void SettingsDialog::PasswordEdited()
{
	passwordManuallyEdited = true;
}
