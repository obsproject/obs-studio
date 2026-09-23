#pragma once

#include <utility/XApiWrappers.hpp>

#include <QDialog>

class QCheckBox;
class QComboBox;
class QLabel;
class QLineEdit;
class QPushButton;

class OBSXBroadcastActions : public QDialog {
	Q_OBJECT

	XApiWrappers *api = nullptr;
	bool valid = false;

	QLineEdit *titleEdit = nullptr;
	QLabel *sourceLabel = nullptr;
	QLabel *status = nullptr;
	QCheckBox *lowLatency = nullptr;
	QCheckBox *noTweet = nullptr;
	QComboBox *chatOption = nullptr;
	QPushButton *goLiveButton = nullptr;

	void BuildUi();
	void ReloadSource();
	void GoLive();

signals:
	void ready();

public:
	explicit OBSXBroadcastActions(QWidget *parent, Auth *auth);

	bool Valid() const { return valid; }
};
