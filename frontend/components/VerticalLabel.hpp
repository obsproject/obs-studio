#pragma once

#include <QLabel>
#include <QPaintEvent>

class VerticalLabel : public QLabel {
	Q_OBJECT

private:
	bool vertical = false;

public:
	VerticalLabel(const QString &text, QWidget *parent = nullptr);
	void ShowNormal();
	void ShowVertical();

protected:
	virtual void paintEvent(QPaintEvent *event) override;
	virtual QSize sizeHint() const override;
	virtual QSize minimumSizeHint() const override;
};
