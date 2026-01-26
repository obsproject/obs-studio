/******************************************************************************
    Copyright (C) 2023 by Lain Bailey <lain@obsproject.com>

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

#include <QImage>
#include <QObject>

#include <thread>

class ScreenshotObj : public QObject {
	Q_OBJECT

public:
	ScreenshotObj(obs_source_t *source);
	~ScreenshotObj() override;
	void Screenshot();
	void Download();
	void Copy();
	void MuxAndFinish();

	gs_texrender_t *texrender = nullptr;
	gs_stagesurf_t *stagesurf = nullptr;
	OBSWeakSource weakSource;
	std::string path;
	QImage image;
	std::vector<uint8_t> half_bytes;
	uint32_t cx;
	uint32_t cy;
	std::thread th;

	int stage = 0;

public slots:
	void Save();
};
