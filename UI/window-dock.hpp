#pragma once

#include <QDockWidget>

class QHBoxLayout;
class QLabel;
class QPushButton;

class OBSDock : public QDockWidget {
	Q_OBJECT

private:
	QLabel *title = nullptr;
	QHBoxLayout *layout = nullptr;
	QPushButton *menuButton = nullptr;

private slots:
	void ShowControlMenu();
	void ToggleFloating();

public:
	void EnableControlMenu(bool lock);
	void AddControlMenu();

	void SetTitle(const QString &newTitle);
	void AddButton(const QString &themeID, const QString &toolTip,
		       const QObject *receiver, const char *slot);
	void AddSeparator();

	OBSDock(QWidget *parent = nullptr);
	virtual void closeEvent(QCloseEvent *event);
};
