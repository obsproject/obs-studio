#pragma once

#include <QDialog>
#include <QString>
#include <QThread>

#include "ui_OBSYoutubeActions.h"
#include "youtube-api-wrappers.hpp"

class WorkerThread : public QThread {
	Q_OBJECT
public:
	WorkerThread(YoutubeApiWrappers *api) : QThread(), apiYouTube(api) {}

	void stop() { pending = false; }

protected:
	YoutubeApiWrappers *apiYouTube;
	bool pending = true;

public slots:
	void run() override;
signals:
	void ready();
	void new_item(const QString &title, const QString &dateTimeString,
		      const QString &broadcast, const QString &status,
		      bool astart, bool astop);
	void failed();
};

class OBSYoutubeActions : public QDialog {
	Q_OBJECT

	std::unique_ptr<Ui::OBSYoutubeActions> ui;

signals:
	void ok(const QString &id, const QString &key, bool autostart,
		bool autostop);

protected:
	void UpdateOkButtonStatus();

	bool StreamNowAction(YoutubeApiWrappers *api,
			     StreamDescription &stream);
	bool StreamLaterAction(YoutubeApiWrappers *api);
	bool ChooseAnEventAction(YoutubeApiWrappers *api,
				 StreamDescription &stream);

	void ShowErrorDialog(QWidget *parent, QString text);

public:
	explicit OBSYoutubeActions(QWidget *parent, Auth *auth);
	virtual ~OBSYoutubeActions() override;

	bool Valid() { return valid; };

private:
	void InitBroadcast();
	void UiToBroadcast(BroadcastDescription &broadcast);
	void OpenYouTubeDashboard();
	void Cancel();
	void Accept();

	QString selectedBroadcast;
	bool autostart, autostop;
	bool valid = false;
	YoutubeApiWrappers *apiYouTube;
	WorkerThread *workerThread;
};
