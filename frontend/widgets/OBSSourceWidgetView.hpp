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
#include <widgets/OBSQTDisplay.hpp>

#include <QFrame>
#include <QVBoxLayout>

class OBSSourceWidget;

class OBSSourceWidgetView : public OBSQTDisplay {
	Q_OBJECT

private:
	OBSWeakSource weakSource = nullptr;

	static void obsRender(void *data, uint32_t cx, uint32_t cy);

	QRect prevGeometry;

	int32_t sourceWidth_;
	int32_t sourceHeight_;

	bool forceLinearSRGB;

public:
	OBSSourceWidgetView(OBSSourceWidget *parent, obs_source_t *source);
	~OBSSourceWidgetView();

	void setSource(obs_source_t *source);
	void setSourceWidth(int width);
	void setSourceHeight(int height);
	int sourceWidth() const { return sourceWidth_; }
	int sourceHeight() const { return sourceHeight_; }

	void setForceLinearSRGB(bool enable);

	OBSSource getSource();

signals:
	void viewReady();
};
