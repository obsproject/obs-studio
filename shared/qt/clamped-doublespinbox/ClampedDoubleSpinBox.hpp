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

#pragma once

#include <QDoubleSpinBox>

class ClampedDoubleSpinBox : public QDoubleSpinBox {
public:
	explicit ClampedDoubleSpinBox(QWidget *parent = nullptr);
	~ClampedDoubleSpinBox();

	QString textFromValue(double value) const override;

public slots:
	void setValue(double val);

private:
	bool isFocused = false;

	int countDecimals(const QString &text);

	int decimalsToDisplay_ = decimals();
	int decimalsToDisplay() const { return std::min(decimals(), decimalsToDisplay_); }

protected:
	QValidator::State validate(QString &text, int &pos) const override;
	void fixup(QString &inputText);

	void focusInEvent(QFocusEvent *event) override;
	void focusOutEvent(QFocusEvent *event) override;
	void keyPressEvent(QKeyEvent *event) override;

	void stepBy(int steps) override;
	void clampCursorPosition();
};
