/******************************************************************************
    Copyright (C) 2025 by Taylor Giampaolo <warchamp7@obsproject.com>

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

#include <QKeyEvent>
#include <QLineEdit>
#include "ClampedDoubleSpinBox.hpp"

namespace {
QString trimAffixes(const QString &fullText, const QString &prefix, const QString &suffix)
{
	if (fullText.isEmpty()) {
		return QString();
	}

	QString text = fullText;

	if (!prefix.isEmpty() && text.startsWith(prefix))
		text.remove(0, prefix.length());

	if (!suffix.isEmpty() && text.endsWith(suffix))
		text.chop(suffix.length());

	return text.trimmed();
}
int countDecimals(const QString &text)
{
	int dotIndex = text.indexOf('.');
	if (dotIndex == -1) {
		return 0;
	}

	int decimalCount = text.length() - dotIndex - 1;
	return decimalCount;
}
} // namespace

ClampedDoubleSpinBox::ClampedDoubleSpinBox(QWidget *parent) : QDoubleSpinBox(parent)
{
	setKeyboardTracking(false);

	connect(lineEdit(), &QLineEdit::textChanged, this, [=]() {
		QString text = trimAffixes(lineEdit()->text(), prefix(), suffix());
		bool result = lineEdit()->blockSignals(true);
		if (text.contains("..")) {
			int pos = text.indexOf("..");
			lineEdit()->setText(text.replace("..", "."));
			lineEdit()->setCursorPosition(pos + 1);

		} else if (text.contains("--")) {
			int pos = text.indexOf("--");
			lineEdit()->setText(text.replace("--", "-"));
			lineEdit()->setCursorPosition(pos + 1);
		}

		if (countDecimals(text) > decimals() && lineEdit()->cursorPosition() > text.indexOf(".")) {
			int pos = lineEdit()->cursorPosition();
			lineEdit()->setText(text.sliced(0, text.length() - 1));
			lineEdit()->setCursorPosition(pos);
		}
		lineEdit()->blockSignals(result);
	});

	connect(lineEdit(), &QLineEdit::cursorPositionChanged, this, [=]() { clampCursorPosition(); });
}

ClampedDoubleSpinBox::~ClampedDoubleSpinBox() {}

QValidator::State ClampedDoubleSpinBox::validate(QString &text, int &pos) const
{
	QString trimmed = trimAffixes(text, prefix(), suffix());

	bool ok;
	trimmed.toDouble(&ok);

	/* Permit empty textbox */
	if (text.isEmpty()) {
		return QValidator::Intermediate;
	}

	/* Allow typing a dot or dash "over" an existing one */
	if (text.contains("..") || text.contains("--")) {
		return QValidator::Intermediate;
	}

	/* Only allow a single "." or "-" */
	if (text.count(".") > 1 || text.count("-") > 1) {
		return QValidator::Invalid;
	}

	/* Only allow "-" at the start of the text */
	if (text.count("-") == 1 && !text.startsWith("-")) {
		return QValidator::Invalid;
	}

	/* Sane limit on total characters */
	int maxLength =
		std::max(QString::number((int)abs(maximum())).length(), QString::number((int)abs(minimum())).length());
	maxLength += 1;
	maxLength += decimals();
	maxLength += trimmed.startsWith("-") ? 1 : 0;
	if (trimmed.indexOf(".") >= 0) {
		maxLength += 1;

		if (decimals() > 0 && lineEdit()->cursorPosition() - 1 > trimmed.indexOf(".")) {
			maxLength += 1;
		}
	}

	if (trimmed.length() > maxLength) {
		return QValidator::Invalid;
	}

	/* Run normal validator for any non-double */
	if (!ok) {
		return QDoubleSpinBox::validate(text, pos);
	}

	return QValidator::Intermediate;
}

void ClampedDoubleSpinBox::focusInEvent(QFocusEvent *event)
{
	/* Trim trailing zeros */
	QString text = trimAffixes(lineEdit()->text(), prefix(), suffix());
	int dotIndex = text.indexOf('.');
	if (dotIndex != -1) {
		while (text.endsWith('0') && text.length() > dotIndex + 2) {
			text.chop(1);
		}
	}
	lineEdit()->setText(text);

	QDoubleSpinBox::focusInEvent(event);
}

void ClampedDoubleSpinBox::focusOutEvent(QFocusEvent *event)
{
	clampAndSetText();
	setValue(valueFromText(lineEdit()->text()));
	QDoubleSpinBox::focusOutEvent(event);
}

void ClampedDoubleSpinBox::keyPressEvent(QKeyEvent *event)
{
	if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
		int cursor = lineEdit()->cursorPosition();
		clampAndSetText();
		setValue(valueFromText(lineEdit()->text()));
		lineEdit()->setCursorPosition(cursor);
		return;
	}
	QDoubleSpinBox::keyPressEvent(event);
}

void ClampedDoubleSpinBox::stepBy(int steps)
{
	clampAndSetText();
	setValue(valueFromText(lineEdit()->text()));
	QDoubleSpinBox::stepBy(steps);
}

void ClampedDoubleSpinBox::clampCursorPosition()
{
	if (suffix().isEmpty() && prefix().isEmpty()) {
		return;
	}

	if (!suffix().isEmpty()) {
		int suffixIndex = lineEdit()->text().length() - suffix().length();
		bool isCursorAfterSuffix = lineEdit()->cursorPosition() > suffixIndex;
		if (isCursorAfterSuffix) {
			lineEdit()->setCursorPosition(suffixIndex);
			return;
		}
	}

	if (!prefix().isEmpty()) {
		int prefixIndex = lineEdit()->text().length() - prefix().length();
		bool isCursorAfterPrefix = lineEdit()->cursorPosition() < prefixIndex;
		if (isCursorAfterPrefix) {
			lineEdit()->setCursorPosition(prefixIndex);
			return;
		}
	}
}

void ClampedDoubleSpinBox::clampAndSetText()
{
	QString text = trimAffixes(lineEdit()->text(), prefix(), suffix());

	if (countDecimals(text) > decimals()) {
		text.chop(1);
	}

	bool ok;
	double val = text.toDouble(&ok);
	if (!ok) {
		/* Restore existing value if text is not a valid double */
		lineEdit()->setText(textFromValue(value()));
		return;
	}

	/* Skip formatting when text ends with a decimal */
	if (text.indexOf(".") == text.length() - 1) {
		return;
	}

	double clamped = std::clamp(val, minimum(), maximum());

	QString formatted = QString::number(clamped, 'f', decimals());

	int cursor = lineEdit()->cursorPosition() + std::max(0, ((int)formatted.length() - (int)text.length()));
	lineEdit()->setText(formatted);
	lineEdit()->setCursorPosition(cursor);
	clampCursorPosition();
}
