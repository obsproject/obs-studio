/******************************************************************************
 Copyright (C) 2025 by Patrick Heyer <PatTheMav@users.noreply.github.com>

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

import CoreVideo

extension obs_video_info {
    var coreVideoFormat: OSType? {
        switch self.output_format {
        case VIDEO_FORMAT_I420:
            if self.range == VIDEO_RANGE_FULL {
                return kCVPixelFormatType_420YpCbCr8PlanarFullRange
            } else {
                return kCVPixelFormatType_420YpCbCr8Planar
            }
        case VIDEO_FORMAT_NV12:
            if self.range == VIDEO_RANGE_FULL {
                return kCVPixelFormatType_420YpCbCr8BiPlanarFullRange
            } else {
                return kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange
            }
        case VIDEO_FORMAT_UYVY:
            if self.range == VIDEO_RANGE_FULL {
                return kCVPixelFormatType_422YpCbCr8FullRange
            } else {
                return kCVPixelFormatType_422YpCbCr8
            }
        case VIDEO_FORMAT_P010:
            if self.range == VIDEO_RANGE_FULL {
                return kCVPixelFormatType_420YpCbCr10BiPlanarFullRange
            } else {
                return kCVPixelFormatType_420YpCbCr10BiPlanarVideoRange
            }
        default:
            return nil
        }
    }
}
