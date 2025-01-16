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

#include "OBSPreviewScalingLabel.hpp"
#include "moc_OBSPreviewScalingLabel.cpp"

void OBSPreviewScalingLabel::PreviewScaleChanged(float scale)
{
	previewScale = scale;
	UpdateScaleLabel();
}

void OBSPreviewScalingLabel::UpdateScaleLabel()
{
	float previewScalePercent = floor(100.0f * previewScale);
	setText(QString::number(previewScalePercent) + "%");
}
