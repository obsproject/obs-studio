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

#include <ClampedDoubleSpinBox.hpp>

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
} // namespace

ClampedDoubleSpinBox::ClampedDoubleSpinBox(QWidget *parent) : QDoubleSpinBox(parent)
{
	setKeyboardTracking(false);

	connect(lineEdit(), &QLineEdit::textChanged, this, [=]() {
		QSignalBlocker block(lineEdit());

		QString text = cleanText();

		if (text.contains(locale().decimalPoint() + locale().decimalPoint())) {
			int pos = text.indexOf(locale().decimalPoint() + locale().decimalPoint());
			lineEdit()->setText(text.replace(locale().decimalPoint() + locale().decimalPoint(),
							 locale().decimalPoint()));
			lineEdit()->setCursorPosition(pos + 1);
		} else if (text.contains("..")) {
			int pos = text.indexOf("..");
			lineEdit()->setText(text.replace("..", "."));
			lineEdit()->setCursorPosition(pos + 1);
		} else if (text.contains("--")) {
			int pos = text.indexOf("--");
			lineEdit()->setText(text.replace("--", "-"));
			lineEdit()->setCursorPosition(pos + 1);
		}

		if (countDecimals(text) > decimals()) {
			if (lineEdit()->cursorPosition() > text.indexOf(locale().decimalPoint()) ||
			    lineEdit()->cursorPosition() > text.indexOf(".")) {
				int pos = lineEdit()->cursorPosition();
				lineEdit()->setText(text.sliced(0, text.length() - 1));
				lineEdit()->setCursorPosition(pos);
			}
		}
	});

	connect(lineEdit(), &QLineEdit::cursorPositionChanged, this, [=]() { clampCursorPosition(); });
}

ClampedDoubleSpinBox::~ClampedDoubleSpinBox() {}

int ClampedDoubleSpinBox::countDecimals(const QString &text)
{
	int dotIndex = text.indexOf(".");
	int decimalIndex = text.indexOf(locale().decimalPoint());
	if (dotIndex == -1 && decimalIndex == -1) {
		return 0;
	}

	int decimalCount = 0;
	if (dotIndex >= 0) {
		decimalCount = text.length() - dotIndex - 1;
	} else if (decimalIndex >= 0) {
		decimalCount = text.length() - decimalIndex - 1;
	}

	return decimalCount;
}

void ClampedDoubleSpinBox::setValue(double val)
{
	if (!isFocused) {
		decimalsToDisplay_ = decimals();
	}

	QDoubleSpinBox::setValue(val);
}

QString ClampedDoubleSpinBox::textFromValue(double value) const
{
	QLocale loc = locale();
	if (isGroupSeparatorShown()) {
		loc.setNumberOptions(loc.numberOptions() & ~QLocale::OmitGroupSeparator);
	} else {
		loc.setNumberOptions(loc.numberOptions() | QLocale::OmitGroupSeparator);
	}

	return loc.toString(value, 'f', decimalsToDisplay());
}

QValidator::State ClampedDoubleSpinBox::validate(QString &text_, int &pos) const
{
	QString trimmed = trimAffixes(text_, prefix(), suffix());
	trimmed = trimmed.replace(locale().decimalPoint(), ".");

	bool ok;
	trimmed.toDouble(&ok);

	/* Permit empty textbox */
	if (trimmed.isEmpty()) {
		return QValidator::Intermediate;
	}

	/* Allow typing a dot or dash "over" an existing one */
	if (trimmed.contains("..") || trimmed.contains("--")) {
		return QValidator::Intermediate;
	}

	/* Only allow a single decimal or - */
	if (trimmed.count(".") > 1 || trimmed.count("-") > 1) {
		return QValidator::Invalid;
	}

	/* Only allow "-" at the start of the text */
	if (trimmed.count("-") == 1 && !trimmed.startsWith("-")) {
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
		return QDoubleSpinBox::validate(text_, pos);
	}

	return QValidator::Intermediate;
}

void ClampedDoubleSpinBox::fixup(QString &inputText)
{
	inputText.remove(locale().groupSeparator());

	inputText = trimAffixes(inputText, prefix(), suffix());
	inputText = inputText.replace(locale().decimalPoint(), ".");

	if (countDecimals(inputText) > decimals()) {
		inputText.chop(1);
	}

	bool ok;
	double val = inputText.toDouble(&ok);
	if (!ok) {
		/* Restore existing value if text is not a valid double */
		lineEdit()->setText(textFromValue(value()));
		return;
	}

	/* Skip formatting when text ends with a decimal */
	if (inputText.indexOf(".") == inputText.length() - 1) {
		return;
	}

	decimalsToDisplay_ = std::min(countDecimals(inputText), decimals());

	double clamped = std::clamp(val, minimum(), maximum());
	QString formatted = textFromValue(clamped);

	QSignalBlocker blocker(lineEdit());
	lineEdit()->setText(formatted);
	clampCursorPosition();
}

void ClampedDoubleSpinBox::focusInEvent(QFocusEvent *event)
{
	isFocused = true;

	QDoubleSpinBox::focusInEvent(event);
}

void ClampedDoubleSpinBox::focusOutEvent(QFocusEvent *event)
{
	QString text = lineEdit()->text();
	fixup(text);
	setValue(valueFromText(text));

	QDoubleSpinBox::focusOutEvent(event);

	isFocused = false;
}

void ClampedDoubleSpinBox::keyPressEvent(QKeyEvent *event)
{
	int cursor = lineEdit()->cursorPosition();

	switch (event->key()) {
	case Qt::Key_Return:
	case Qt::Key_Enter:
		setValue(valueFromText(lineEdit()->text()));
		lineEdit()->setCursorPosition(cursor);
		event->accept();
		return;
	}

	QDoubleSpinBox::keyPressEvent(event);
}

void ClampedDoubleSpinBox::stepBy(int steps)
{
	QString stepAmount = QString::number(singleStep() * steps);
	int stepDecimals = countDecimals(stepAmount);

	QString currentText = lineEdit()->text();
	fixup(currentText);
	int currentDecimals = countDecimals(currentText);
	decimalsToDisplay_ = std::max(stepDecimals, currentDecimals);

	/* Update spinbox value before performing step update */
	const bool wasBlocked = blockSignals(true);
	setValue(valueFromText(currentText));
	blockSignals(wasBlocked);

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
