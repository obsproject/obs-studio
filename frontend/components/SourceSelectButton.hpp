/******************************************************************************
    Copyright (C) 2025 by Taylor Giampaolo <warchamp7@obsproject.com>
                          Lain Bailey <lain@obsproject.com>

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

#include <QLabel>
#include <QPointer>
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>

class QLabel;
class Thumbnail;
class ThumbnailView;

class SourceSelectButton : public QAbstractButton {
	Q_OBJECT

public:
	SourceSelectButton(OBSWeakSource source, QWidget *parent = nullptr);
	~SourceSelectButton();

	std::string uuid() const { return sourceUuid; };

	void setThumbnailEnabled(bool enabled);
	void updateThumbnail();

protected:
	void paintEvent(QPaintEvent *event) override;
	void resizeEvent(QResizeEvent *event) override;
	void enterEvent(QEnterEvent *event) override;
	void leaveEvent(QEvent *event) override;
	void mouseMoveEvent(QMouseEvent *event) override;
	void buttonPressed();

private:
	OBSWeakSource weakSource;
	QPointer<ThumbnailView> thumbnail;
	QPointer<QLabel> image;
	std::string sourceUuid;

	std::vector<OBSSignal> signalHandlers;
	static void obsSourceRemoved(void *param, calldata_t *calldata);
	static void obsSourceRenamed(void *param, calldata_t *calldata);

	QLabel *label = nullptr;
	bool thumbnailEnabled = true;

	QPoint dragStartPosition;

private slots:
	void updatePixmap(QPixmap pixmap);
	void handleSourceRemoved();
	void handleSourceRenamed(QString name);

signals:
	void sourceRemoved();
};
