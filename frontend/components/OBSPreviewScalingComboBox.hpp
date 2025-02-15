/******************************************************************************
    Copyright (C) 2024 by Taylor Giampaolo <warchamp7@obsproject.com>

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

#include <QComboBox>

class OBSPreviewScalingComboBox : public QComboBox {
	Q_OBJECT

public:
	OBSPreviewScalingComboBox(QWidget *parent = nullptr) : QComboBox(parent) {}

	inline void SetCanvasSize(uint32_t width, uint32_t height)
	{
		canvas_width = width;
		canvas_height = height;
	};
	inline void SetOutputSize(uint32_t width, uint32_t height)
	{
		output_width = width;
		output_height = height;
	};
	void UpdateAllText();

public slots:
	void PreviewScaleChanged(float scale);
	void PreviewFixedScalingChanged(bool fixed);
	void CanvasResized(uint32_t width, uint32_t height);
	void OutputResized(uint32_t width, uint32_t height);

private:
	uint32_t canvas_width = 0;
	uint32_t canvas_height = 0;

	uint32_t output_width = 0;
	uint32_t output_height = 0;

	float previewScale = 0.0f;

	bool fixedScaling = false;

	bool scaleOutputEnabled = false;
	void SetScaleOutputEnabled(bool show);

	void UpdateCanvasText();
	void UpdateOutputText();
	void UpdateScaledText();
	void UpdateSelection();
};
