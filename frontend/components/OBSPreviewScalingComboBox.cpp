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

#include "OBSPreviewScalingComboBox.hpp"

#include <OBSApp.hpp>

#include "moc_OBSPreviewScalingComboBox.cpp"

void OBSPreviewScalingComboBox::PreviewFixedScalingChanged(bool fixed)
{
	if (fixedScaling == fixed)
		return;

	fixedScaling = fixed;
	UpdateSelection();
}

void OBSPreviewScalingComboBox::CanvasResized(uint32_t width, uint32_t height)
{
	SetCanvasSize(width, height);
	UpdateCanvasText();
}

void OBSPreviewScalingComboBox::OutputResized(uint32_t width, uint32_t height)
{
	SetOutputSize(width, height);

	bool canvasMatchesOutput = output_width == canvas_width && output_height == canvas_height;

	SetScaleOutputEnabled(!canvasMatchesOutput);
	UpdateOutputText();
}

void OBSPreviewScalingComboBox::PreviewScaleChanged(float scale)
{
	previewScale = scale;

	if (fixedScaling) {
		UpdateSelection();
		UpdateAllText();
	} else {
		UpdateScaledText();
	}
}

void OBSPreviewScalingComboBox::SetScaleOutputEnabled(bool show)
{
	if (scaleOutputEnabled == show)
		return;

	scaleOutputEnabled = show;

	if (scaleOutputEnabled) {
		addItem(QTStr("Basic.MainMenu.Edit.Scale.Output"));
	} else {
		removeItem(2);
	}
}

void OBSPreviewScalingComboBox::UpdateAllText()
{
	UpdateCanvasText();
	UpdateOutputText();
	UpdateScaledText();
}

void OBSPreviewScalingComboBox::UpdateCanvasText()
{
	QString text = QTStr("Basic.MainMenu.Edit.Scale.Canvas");
	text = text.arg(QString::number(canvas_width), QString::number(canvas_height));
	setItemText(1, text);
}

void OBSPreviewScalingComboBox::UpdateOutputText()
{
	if (scaleOutputEnabled) {
		QString text = QTStr("Basic.MainMenu.Edit.Scale.Output");
		text = text.arg(QString::number(output_width), QString::number(output_height));
		setItemText(2, text);
	}
}

void OBSPreviewScalingComboBox::UpdateScaledText()
{
	QString text = QTStr("Basic.MainMenu.Edit.Scale.Manual");
	text = text.arg(QString::number(floor(canvas_width * previewScale)),
			QString::number(floor(canvas_height * previewScale)));
	setPlaceholderText(text);
}

void OBSPreviewScalingComboBox::UpdateSelection()
{
	QSignalBlocker sb(this);
	float outputScale = float(output_width) / float(canvas_width);

	if (!fixedScaling) {
		setCurrentIndex(0);
	} else {
		if (previewScale == 1.0f) {
			setCurrentIndex(1);
		} else if (scaleOutputEnabled && (previewScale == outputScale)) {
			setCurrentIndex(2);
		} else {
			setCurrentIndex(-1);
		}
	}
}
