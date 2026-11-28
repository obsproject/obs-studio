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

#pragma once

#include <components/OBSSourceLabel.hpp>

#include <obs.hpp>

#include <QLineEdit>
#include <QPointer>

class QLabel;
class QPropertyAnimation;
class QTimer;

class OBSInlineNameEditor : public QLineEdit {
	Q_OBJECT

	QString originalName;
	OBSSource source;

	std::vector<OBSSignal> obsSignals;

	bool wasFocusedFromMouseClick = false;

	QPropertyAnimation *errorAnimation{nullptr};
	void playErrorAnimation();

	void clearSource();

	QString pendingName;
	void clearPendingName();

public:
	OBSInlineNameEditor(OBSSource source, QWidget *parent = nullptr);

	void setSource(OBSSource source);

	void beginEditing();
	void finishEditing(bool save);

protected:
	static void obsSourceRenamed(void *data, calldata_t *params);
	static void obsSourceRemoved(void *data, calldata_t *params);
	static void obsSourceDestroyed(void *data, calldata_t *params);

	void focusInEvent(QFocusEvent *event) override;
	void focusOutEvent(QFocusEvent *event) override;

	void enterEvent(QEnterEvent *event) override;
	void mousePressEvent(QMouseEvent *event) override;

	void keyPressEvent(QKeyEvent *event) override;

signals:
	void editingFinished();

private slots:
	void setOriginalName(QString name);
	void setPendingName(QString name);
};
