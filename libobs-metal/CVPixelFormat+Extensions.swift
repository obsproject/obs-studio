/******************************************************************************
 Copyright (C) 2024 by Patrick Heyer <PatTheMav@users.noreply.github.com>

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
import Metal

extension OSType {
    /// Conversion of CoreVideo pixel formats into corresponding Metal pixel formats
    var mtlFormat: MTLPixelFormat? {
        switch self {
        case kCVPixelFormatType_OneComponent8:
            return .r8Unorm
        case kCVPixelFormatType_OneComponent16Half:
            return .r16Float
        case kCVPixelFormatType_OneComponent32Float:
            return .r32Float
        case kCVPixelFormatType_TwoComponent8:
            return .rg8Unorm
        case kCVPixelFormatType_TwoComponent16Half:
            return .rg16Float
        case kCVPixelFormatType_TwoComponent32Float:
            return .rg32Float
        case kCVPixelFormatType_32BGRA:
            return .bgra8Unorm
        case kCVPixelFormatType_32RGBA:
            return .rgba8Unorm
        case kCVPixelFormatType_64RGBAHalf:
            return .rgba16Float
        case kCVPixelFormatType_128RGBAFloat:
            return .rgba32Float
        case kCVPixelFormatType_ARGB2101010LEPacked:
            return .bgr10a2Unorm
        default:
            return nil
        }
    }
}
