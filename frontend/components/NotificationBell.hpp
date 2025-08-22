#pragma once

#include <QFrame>

class QMouseEvent;
class QPaintEvent;

class NotificationBell : public QFrame {
	Q_OBJECT
	Q_PROPERTY(QIcon icon READ getIcon WRITE setIcon DESIGNABLE true)
	Q_PROPERTY(QColor countColor READ getCountColor WRITE setCountColor DESIGNABLE true)

private:
	int count = 0;
	int importantCount = 0;
	QString text;

	QIcon icon;
	QColor countColor;

	inline QIcon getIcon() { return icon; };
	inline QColor getCountColor() { return countColor; };

private slots:
	inline void setIcon(const QIcon &icon_) { icon = icon_; };
	inline void setCountColor(const QColor &color) { countColor = color; };

public:
	NotificationBell(QWidget *parent = nullptr);

public slots:
	void setCount(int count_, int importantCount_);

protected:
	virtual void paintEvent(QPaintEvent *event) override;
	virtual void mousePressEvent(QMouseEvent *event) override;

signals:
	void clicked();
};
