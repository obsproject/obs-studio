// SPDX-FileCopyrightText: 2021 Yuriy Chumak <yuriy.chumak@mail.com>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QDialog>
#include <QString>
#include <QThread>

#include "ui_OBSYoutubeActions.h"
#include "youtube-oauth.hpp"

class WorkerThread : public QThread {
	Q_OBJECT
public:
	WorkerThread(YouTubeApi::ServiceOAuth *api) : QThread(), apiYouTube(api)
	{
	}

	void stop() { pending = false; }

protected:
	YouTubeApi::ServiceOAuth *apiYouTube;
	bool pending = true;

public slots:
	void run() override;
signals:
	void ready();
	void new_item(const QString &title, const QString &dateTimeString,
		      const QString &broadcast,
		      const YouTubeApi::LiveBroadcastLifeCycleStatus &status,
		      bool astart, bool astop);
	void failed();
};

class OBSYoutubeActions : public QDialog {
	Q_OBJECT
	Q_PROPERTY(QIcon thumbPlaceholder READ GetPlaceholder WRITE
			   SetPlaceholder DESIGNABLE true)

	std::unique_ptr<Ui::OBSYoutubeActions> ui;

	OBSYoutubeActionsSettings *settings;

signals:
	void ok(const QString &id, const QString &key, bool autostart,
		bool autostop, bool start_now);

protected:
	void showEvent(QShowEvent *event) override;
	void UpdateOkButtonStatus();

	bool CreateEventAction(YouTubeApi::LiveStream &stream,
			       bool stream_later, bool ready_broadcast = false);
	bool ChooseAnEventAction(YouTubeApi::LiveStream &stream);

	void ShowErrorDialog(QWidget *parent, QString text);

public:
	explicit OBSYoutubeActions(QWidget *parent,
				   YouTubeApi::ServiceOAuth *auth,
				   OBSYoutubeActionsSettings *settings,
				   bool broadcastReady);
	virtual ~OBSYoutubeActions() override;

	bool Valid() { return valid; };

private:
	void InitBroadcast();
	void ReadyBroadcast();
	YouTubeApi::LiveBroadcast UiToBroadcast();
	void OpenYouTubeDashboard();
	void Cancel();
	void Accept();
	void SaveSettings();
	void LoadSettings();

	QIcon GetPlaceholder() { return thumbPlaceholder; }
	void SetPlaceholder(const QIcon &icon) { thumbPlaceholder = icon; }

	QString selectedBroadcast;
	bool autostart, autostop;
	bool valid = false;
	YouTubeApi::ServiceOAuth *apiYouTube;
	WorkerThread *workerThread = nullptr;
	bool broadcastReady = false;
	QString thumbnailFilePath;
	QByteArray thumbnailData;
	QIcon thumbPlaceholder;
};
