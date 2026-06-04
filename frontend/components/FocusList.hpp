#pragma once

#include <QListWidget>

class FocusList : public QListWidget {
	Q_OBJECT

public:
	FocusList(QWidget *parent);

protected:
	void focusInEvent(QFocusEvent *event) override;
	virtual void dragMoveEvent(QDragMoveEvent *event) override;

signals:
	void GotFocus();
};
