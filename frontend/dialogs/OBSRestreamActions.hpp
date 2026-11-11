#pragma once

#include "ui_OBSRestreamActions.h"

#include <oauth/RestreamAuth.hpp>

class OBSRestreamActions : public QDialog {
	Q_OBJECT

	std::unique_ptr<Ui::OBSRestreamActions> ui;

signals:
	void ok(bool startNow);

protected:
	void EnableOkButton(bool state);

public:
	explicit OBSRestreamActions(QWidget *parent, Auth *auth, bool broadcastReady);
	virtual ~OBSRestreamActions() override;

private:
	void UpdateBroadcastList(QVector<RestreamEventDescription> &newEvents);
	void BroadcastSelectAction();
	void BroadcastSelectAndStartAction();
	void OpenRestreamDashboard();

	RestreamAuth *restreamAuth;
	std::string selectedEventId;
	std::string selectedShowId;
	bool broadcastReady;
};
