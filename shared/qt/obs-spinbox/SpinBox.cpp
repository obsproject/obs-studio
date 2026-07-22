/******************************************************************************
    Copyright (C) 2026 by Taylor Giampaolo <warchamp7@obsproject.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/

#include "SpinBox.hpp"

#include "AutoSelectLineEdit.hpp"

#include <QApplication>
#include <QKeyEvent>
#include <QLocale>
#include <QMouseEvent>
#include <QStyle>
#include <QStyleOptionSpinBox>
#include <QTimer>

namespace {
QString trimAffixes(const QString &fullText, const QString &prefix, const QString &suffix)
{
	if (fullText.isEmpty()) {
		return QString();
	}

	QString text = fullText;

	if (!prefix.isEmpty() && text.startsWith(prefix)) {
		text.remove(0, prefix.length());
	}

	if (!suffix.isEmpty() && text.endsWith(suffix)) {
		text.chop(suffix.length());
	}

	return text.trimmed();
}
} // namespace

namespace OBS {
SpinBox::SpinBox(QWidget *parent) : QSpinBox(parent)
{
	setKeyboardTracking(false);
	setFocusPolicy(Qt::StrongFocus);
	auto lineEdit = new AutoSelectLineEdit(this);
	setLineEdit(lineEdit);

	connect(lineEdit, &QLineEdit::cursorPositionChanged, this, &SpinBox::clampCursorPosition);
}

SpinBox::~SpinBox() {}

QValidator::State SpinBox::validate(QString &text, int &) const
{
	QString stripped = trimAffixes(text, prefix(), suffix());

	if (stripped.isEmpty() || stripped == "-") {
		return QValidator::Intermediate;
	}

	bool ok;
	int val = locale().toInt(stripped, &ok);

	if (!ok) {
		return QValidator::Invalid;
	}

	QString resultText = prefix() + textFromValue(val) + suffix();
	if (text != resultText) {
		return QValidator::Intermediate;
	}

	if (val < minimum() || val > maximum()) {
		return QValidator::Intermediate;
	}

	return QValidator::Acceptable;
}

void SpinBox::fixup(QString &input) const
{
	QString stripped = trimAffixes(input, prefix(), suffix());

	bool ok;
	int val = locale().toInt(stripped, &ok);

	if (ok) {
		val = qBound(minimum(), val, maximum());
		input = prefix() + textFromValue(val) + suffix();
	} else {
		// Fallback to base class behavior if parsing fails
		QSpinBox::fixup(input);
	}
}

void SpinBox::mousePressEvent(QMouseEvent *event)
{
	QStyleOptionSpinBox opt;
	initStyleOption(&opt);

	if (style()->subControlRect(QStyle::CC_SpinBox, &opt, QStyle::SC_SpinBoxUp, this).contains(event->pos())) {
		QSpinBox::mousePressEvent(event);
		return;
	}

	if (style()->subControlRect(QStyle::CC_SpinBox, &opt, QStyle::SC_SpinBoxDown, this).contains(event->pos())) {
		QSpinBox::mousePressEvent(event);
		return;
	}

	// Mouse button was pressed and is and not over the spinbox buttons, forward event to the lineEdit.
	QRect lineEditRect = lineEdit()->geometry();
	mouseDragYOffset = ((double)height() / 2) - event->position().y();

	if (!lineEditRect.contains(event->pos())) {
		QPointF localPos = event->pos() - lineEditRect.topLeft();
		QMouseEvent forwardEvent(event->type(), localPos, event->globalPosition(), event->button(),
					 event->buttons(), event->modifiers());

		QCoreApplication::sendEvent(lineEdit(), &forwardEvent);
		return;
	}

	QSpinBox::mousePressEvent(event);
}

void SpinBox::mouseMoveEvent(QMouseEvent *event)
{
	if (!(event->buttons() & Qt::LeftButton)) {
		QSpinBox::mouseMoveEvent(event);
		return;
	}

	QStyleOptionSpinBox opt;
	initStyleOption(&opt);

	if (style()->subControlRect(QStyle::CC_SpinBox, &opt, QStyle::SC_SpinBoxUp, this).contains(event->pos())) {
		QSpinBox::mouseMoveEvent(event);
		return;
	}

	if (style()->subControlRect(QStyle::CC_SpinBox, &opt, QStyle::SC_SpinBoxDown, this).contains(event->pos())) {
		QSpinBox::mouseMoveEvent(event);
		return;
	}

	// Mouse button is down and not over the spinbox buttons, forward move event to the lineEdit.
	// This allows events within the spinbox but outside the actual lineEdit widget to work.
	QPointF relativePos = lineEdit()->mapFromGlobal(event->globalPosition());
	relativePos.ry() -= mouseDragYOffset;

	QMouseEvent forwardEvent(event->type(), relativePos, event->globalPosition(), event->button(), event->buttons(),
				 event->modifiers());
	QCoreApplication::sendEvent(lineEdit(), &forwardEvent);
}

void SpinBox::keyPressEvent(QKeyEvent *event)
{
	bool isShiftHeld = QGuiApplication::keyboardModifiers() & Qt::ShiftModifier;

	switch (event->key()) {
	case Qt::Key_Up:
		event->accept();
		if (stepEnabled() & QAbstractSpinBox::StepUpEnabled) {
			stepBy(isShiftHeld ? 10 : 1);
		}
		return;
	case Qt::Key_Down:
		event->accept();
		if (stepEnabled() & QAbstractSpinBox::StepDownEnabled) {
			stepBy(isShiftHeld ? -10 : -1);
		}
		return;
	case Qt::Key_Return:
	case Qt::Key_Enter:
		event->accept();
		interpretText();
		return;
	}

	QSpinBox::keyPressEvent(event);
}

void SpinBox::wheelEvent(QWheelEvent *event)
{
	if (!hasFocus()) {
		event->ignore();
	} else {
		QSpinBox::wheelEvent(event);
	}
}

void SpinBox::clampCursorPosition()
{
	QSignalBlocker block(lineEdit());

	if (suffix().isEmpty() && prefix().isEmpty()) {
		return;
	}

	int cursorPosition = lineEdit()->cursorPosition();
	bool hasSelectedText = !lineEdit()->selectedText().isEmpty();

	if (!suffix().isEmpty()) {
		int suffixIndex = lineEdit()->text().length() - suffix().length();
		if (cursorPosition > suffixIndex) {
			int cursorOvershoot = lineEdit()->selectionEnd() - suffixIndex;
			if (hasSelectedText && cursorOvershoot > 0) {
				lineEdit()->setSelection(lineEdit()->selectionStart(),
							 lineEdit()->selectionLength() - cursorOvershoot);
			} else {
				lineEdit()->setCursorPosition(suffixIndex);
			}
			return;
		}
	}

	if (!prefix().isEmpty()) {
		int prefixIndex = prefix().length();
		if (cursorPosition < prefixIndex) {
			int cursorUndershoot = prefixIndex - lineEdit()->selectionStart();
			if (hasSelectedText && cursorUndershoot > 0) {
				lineEdit()->setSelection(lineEdit()->selectionEnd(),
							 -lineEdit()->selectionLength() + cursorUndershoot);
			} else {
				lineEdit()->setCursorPosition(prefixIndex);
			}
			return;
		}
	}
}
} // namespace OBS
