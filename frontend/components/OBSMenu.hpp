#pragma once

#include <QMenu>

class OBSMenu : public QMenu {
	Q_OBJECT

	QWidget* parent;

public:
	OBSMenu(QWidget*);

	void showEvent(QShowEvent *event) override;

	void popupMenu();
};
