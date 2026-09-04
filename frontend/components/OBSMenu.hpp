#pragma once

#include <QWidget>
#include <QMenu>

class OBSMenu : public QMenu {
	Q_OBJECT

	QWidget* parent;

public:
	OBSMenu(QWidget*, const bool&);

	OBSMenu(const QString&, QWidget*, const bool&);

	void showEvent(QShowEvent *event) override;

	void popupMenu();
};
