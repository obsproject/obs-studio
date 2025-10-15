#pragma once

#include <Idian/Utils.hpp>

#include <QWidget>
#include <QAbstractbutton>
#include <QStyleOptionButton>

namespace idian {
class StateEventFilter : public QObject {
	Q_OBJECT

public:
	explicit StateEventFilter(idian::Utils *utils, QWidget *parent);

	bool eventFilter(QObject *obj, QEvent *event);

public slots:
	void updateCheckedState(bool checked);

private:
	Utils *utils;
	QWidget *target;
};
} // namespace idian
