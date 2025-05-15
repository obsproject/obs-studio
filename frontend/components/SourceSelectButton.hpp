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

// #include <widgets/OBSSourceWidget.hpp>

#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>

#include <obs.h>

class SourceSelectButton : public QFrame {
	Q_OBJECT

public:
	SourceSelectButton(obs_source_t *source, QWidget *parent = nullptr);
	~SourceSelectButton();

	QPointer<QPushButton> getButton();
	QString text();

protected:
	void resizeEvent(QResizeEvent *event) override;
	void moveEvent(QMoveEvent *event) override;

private:
	obs_source_t *source;

	QPushButton *button = nullptr;
	QVBoxLayout *layout = nullptr;
	QLabel *label = nullptr;

	// OBSSourceWidget *sourceWidget;

private slots:
};
