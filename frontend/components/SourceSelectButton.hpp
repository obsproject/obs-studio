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

class SourceSelectButton : public QFrame {
	Q_OBJECT

public:
	SourceSelectButton(obs_source_t *source, QWidget *parent = nullptr);
	~SourceSelectButton();

	QPointer<QPushButton> getButton();
	QString text();

	void setRectVisible(bool visible);
	void setPreload(bool preload);

protected:
	void resizeEvent(QResizeEvent *event) override;
	void moveEvent(QMoveEvent *event) override;
	void mouseMoveEvent(QMouseEvent *event) override;
	void buttonPressed();

private:
	OBSWeakSource weakSource;
	std::shared_ptr<Thumbnail> thumbnail;
	QPointer<QLabel> image;

	QPushButton *button = nullptr;
	QVBoxLayout *layout = nullptr;
	QLabel *label = nullptr;
	bool preload = true;
	bool rectVisible = false;

	void setDefaultThumbnail();

	QPoint dragStartPosition;

private slots:
	void thumbnailUpdated(QPixmap pixmap);
};
