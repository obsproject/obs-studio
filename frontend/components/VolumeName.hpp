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

#include <obs.hpp>

#include <QAbstractButton>
#include <QLabel>
#include <QPointer>

class VolumeName : public QAbstractButton {
	Q_OBJECT;
	Q_PROPERTY(Qt::Alignment textAlignment READ alignment WRITE setAlignment)

	QPointer<QLabel> label{};
	int indicatorWidth;

public:
	VolumeName(obs_source_t *source, QWidget *parent = nullptr);
	~VolumeName();

	void setAlignment(Qt::Alignment alignment);
	Qt::Alignment alignment() const { return textAlignment; }

	QSize sizeHint() const override;

	void updateLabelText(const QString &name);
	void setText(const QString &text);

protected:
	OBSSignal renamedSignal;
	OBSSignal removedSignal;
	OBSSignal destroyedSignal;

	Qt::Alignment textAlignment = Qt::AlignLeft;

	static void obsSourceRenamed(void *data, calldata_t *params);
	static void obsSourceRemoved(void *data, calldata_t *params);
	static void obsSourceDestroyed(void *data, calldata_t *params);

	void resizeEvent(QResizeEvent *event) override;
	void paintEvent(QPaintEvent *event) override;

private slots:
	void onRenamed(QString name);
	void onRemoved();
	void onDestroyed();

signals:
	void renamed(const char *name);
	void removed();
	void destroyed();
};
