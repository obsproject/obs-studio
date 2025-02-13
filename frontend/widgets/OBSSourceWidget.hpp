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

#include <obs.hpp>

#include <QFrame>

class OBSSourceWidgetView;
class QVBoxLayout;

class OBSSourceWidget : public QFrame {
	Q_OBJECT

private:
	OBSSourceWidgetView *sourceView = nullptr;
	QVBoxLayout *layout;

	double fixedAspectRatio;

public:
	OBSSourceWidget(QWidget *parent);
	OBSSourceWidget(QWidget *parent, obs_source_t *source);
	~OBSSourceWidget();

	void setFixedAspectRatio(double ratio);
	void setSource(obs_source_t *source);

	void setForceLinearSRGB(bool enable);

	void resizeSourceView();

	QSize minimumSizeHint() const override;
	QSize sizeHint() const override;
	bool hasHeightForWidth() const override;
	int heightForWidth(int width) const override;

protected:
	bool eventFilter(QObject *obj, QEvent *event) override;
	void moveEvent(QMoveEvent *event) override;
	void resizeEvent(QResizeEvent *event) override;
};
