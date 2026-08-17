/******************************************************************************
    Copyright (C) 2023 by Lain Bailey <lain@obsproject.com>
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

#include <QImage>
#include <QObject>

#include <thread>

class ScreenshotObj : public QObject {
	Q_OBJECT

public:
	struct Options {
		QSize size{};
		bool outputToFile{true};
	};

	ScreenshotObj(obs_source_t *source, const Options &options);
	ScreenshotObj(obs_source_t *source);
	~ScreenshotObj() override;

	enum class Stage { Render, Download, Output, Finished };

	Stage stage() { return stage_; }

private:
	static void renderTick(void *param, float seconds);
	void processStage();

	void renderScreenshot();
	void downloadData();
	void copyData();
	void saveToFile();
	void muxFile();
	void onFinished();

	OBSWeakSource weakSource;

	Stage stage_ = Stage::Render;
	Options options;

	gs_texrender_t *texrender = nullptr;
	gs_stagesurf_t *stagesurf = nullptr;

	std::string path;
	QImage image;
	std::vector<uint8_t> half_bytes;

	uint32_t sourceWidth = 0;
	uint32_t sourceHeight = 0;
	uint32_t outputWidth = 0;
	uint32_t outputHeight = 0;

	std::thread thread;

signals:
	void imageSaved(std::string path);
	void imageReady(QImage image);

private slots:
	void handleSave();
};
