#pragma once

#include <QDialog>
#include <QString>
#include <QThread>

#include "ui_OBSRestreamActions.h"
#include "oauth/RestreamAuth.hpp"

class OBSEventFilter;

class OBSRestreamActions : public QDialog {
	Q_OBJECT
	Q_PROPERTY(QIcon thumbPlaceholder READ GetPlaceholder WRITE SetPlaceholder DESIGNABLE true)

	std::unique_ptr<Ui::OBSRestreamActions> ui;

signals:
	void ok(bool start_now);

protected:
	void EnableOkButton(bool state);

public:
	explicit OBSRestreamActions(QWidget *parent, Auth *auth, bool broadcastReady);
	virtual ~OBSRestreamActions() override;

	bool Valid() { return valid; };

private:
	void UpdateBroadcastList(QVector<RestreamEventDescription> &newEvents);
	void BroadcastSelectAction();
	void BroadcastSelectAndStartAction();
	void OpenRestreamDashboard();

	QIcon GetPlaceholder() { return thumbPlaceholder; }
	void SetPlaceholder(const QIcon &icon) { thumbPlaceholder = icon; }

	RestreamAuth *restreamAuth;
	OBSEventFilter *eventFilter;
	std::string selectedEventId;
	std::string selectedShowId;
	bool broadcastReady;
	bool valid = false;
	QIcon thumbPlaceholder;
};
