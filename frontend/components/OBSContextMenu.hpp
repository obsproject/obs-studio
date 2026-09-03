#pragma once

#include <QMenu>

class OBSContextMenu : public QMenu {
	Q_OBJECT

public:
	OBSContextMenu(QWidget *parent);

	void showEvent(QShowEvent *event) override;
};
