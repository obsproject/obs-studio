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

#include <QClipboard>
#include <QPainter>
#include <QUrl>
#include <obs-module.h>
#include <qrcodegen.hpp>

#include "ConnectInfo.h"
#include "../obs-websocket.h"
#include "../Config.h"
#include "../utils/Platform.h"

ConnectInfo::ConnectInfo(QWidget *parent) : QDialog(parent, Qt::Dialog), ui(new Ui::ConnectInfo)
{
	ui->setupUi(this);

	connect(ui->copyServerIpButton, &QPushButton::clicked, this, &ConnectInfo::CopyServerIpButtonClicked);
	connect(ui->copyServerPortButton, &QPushButton::clicked, this, &ConnectInfo::CopyServerPortButtonClicked);
	connect(ui->copyServerPasswordButton, &QPushButton::clicked, this, &ConnectInfo::CopyServerPasswordButtonClicked);
}

ConnectInfo::~ConnectInfo()
{
	delete ui;
}

void ConnectInfo::showEvent(QShowEvent *)
{
	RefreshData();
}

void ConnectInfo::RefreshData()
{
	auto conf = GetConfig();
	if (!conf) {
		blog(LOG_ERROR, "[ConnectInfo::showEvent] Unable to retreive config!");
		return;
	}

	QString serverIp = QString::fromStdString(Utils::Platform::GetLocalAddress());
	ui->serverIpLineEdit->setText(serverIp);

	QString serverPort = QString::number(conf->ServerPort);
	ui->serverPortLineEdit->setText(serverPort);

	QString serverPassword;
	if (conf->AuthRequired) {
		ui->copyServerPasswordButton->setEnabled(true);
		serverPassword = QUrl::toPercentEncoding(QString::fromStdString(conf->ServerPassword));
	} else {
		ui->copyServerPasswordButton->setEnabled(false);
		serverPassword = obs_module_text("OBSWebSocket.ConnectInfo.ServerPasswordPlaceholderText");
	}
	ui->serverPasswordLineEdit->setText(serverPassword);

	QString connectString;
	if (conf->AuthRequired)
		connectString = QString("obsws://%1:%2/%3").arg(serverIp).arg(serverPort).arg(serverPassword);
	else
		connectString = QString("obsws://%1:%2").arg(serverIp).arg(serverPort);
	DrawQr(connectString);
}

void ConnectInfo::CopyServerIpButtonClicked()
{
	SetClipboardText(ui->serverIpLineEdit->text());
	ui->serverIpLineEdit->selectAll();
}

void ConnectInfo::CopyServerPortButtonClicked()
{
	SetClipboardText(ui->serverPortLineEdit->text());
	ui->serverPortLineEdit->selectAll();
}

void ConnectInfo::CopyServerPasswordButtonClicked()
{
	SetClipboardText(ui->serverPasswordLineEdit->text());
	ui->serverPasswordLineEdit->selectAll();
}

void ConnectInfo::SetClipboardText(QString text)
{
	QClipboard *clipboard = QGuiApplication::clipboard();
	clipboard->setText(text);
}

void ConnectInfo::DrawQr(QString qrText)
{
	QPixmap map(236, 236);
	map.fill(Qt::white);
	QPainter painter(&map);

	qrcodegen::QrCode qr = qrcodegen::QrCode::encodeText(QT_TO_UTF8(qrText), qrcodegen::QrCode::Ecc::MEDIUM);
	const int s = qr.getSize() > 0 ? qr.getSize() : 1;
	const double w = map.width();
	const double h = map.height();
	const double aspect = w / h;
	const double size = ((aspect > 1.0) ? h : w);
	const double scale = size / (s + 2);
	painter.setPen(Qt::NoPen);
	painter.setBrush(Qt::black);

	for (int y = 0; y < s; y++) {
		for (int x = 0; x < s; x++) {
			const int color = qr.getModule(x, y);
			if (0x0 != color) {
				const double ry1 = (y + 1) * scale;
				const double rx1 = (x + 1) * scale;
				QRectF r(rx1, ry1, scale, scale);
				painter.drawRects(&r, 1);
			}
		}
	}

	ui->qrCodeLabel->setPixmap(map);
}
