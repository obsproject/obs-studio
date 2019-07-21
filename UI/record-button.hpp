#pragma once

#include <QPushButton>

class RecordButton : public QPushButton {
	Q_OBJECT

public:
	inline RecordButton(QWidget *parent = nullptr) : QPushButton(parent) {}

	void resizeEvent(QResizeEvent *event) override;
};
