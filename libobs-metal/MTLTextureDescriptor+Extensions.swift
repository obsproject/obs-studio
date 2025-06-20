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

import Metal

extension MTLTextureDescriptor {

    /// Convenience initializer for a texture descriptor with `libobs` data
    /// - Parameters:
    ///   - type: Metal texture type
    ///   - width: Width of texture
    ///   - height: Height of texture
    ///   - depth: Depth of texture
    ///   - colorFormat: `libobs` color format for the texture
    ///   - levels: Mip map levels
    ///   - flags: Additional usage flags as `libobs` bitfield
    convenience init?(
        type: MTLTextureType, width: UInt32, height: UInt32, depth: UInt32, colorFormat: gs_color_format,
        levels: UInt32, flags: UInt32
    ) {
        let arrayLength: Int
        switch type {
        case .type2D:
            arrayLength = 1
        case .type3D:
            arrayLength = 1
        case .typeCube:
            arrayLength = 6
        default:
            assertionFailure("MTLTextureDescriptor: Unsupported texture type for libobs initializer")
            return nil
        }

        self.init()

        self.textureType = type
        self.pixelFormat = colorFormat.mtlFormat
        self.width = Int(width)
        self.height = Int(height)
        self.depth = Int(depth)
        self.sampleCount = 1
        self.arrayLength = arrayLength
        self.cpuCacheMode = .defaultCache
        self.allowGPUOptimizedContents = true
        self.hazardTrackingMode = .default

        if (Int32(flags) & GS_BUILD_MIPMAPS) != 0 {
            self.mipmapLevelCount = Int(levels)
        } else {
            self.mipmapLevelCount = 1
        }

        if (Int32(flags) & GS_RENDER_TARGET) != 0 {
            self.storageMode = .private
            self.usage = [.shaderRead, .renderTarget]
        } else {
            self.storageMode = .shared
            self.usage = [.shaderRead]
        }
    }

    convenience init?(width: UInt32, height: UInt32, colorFormat: gs_zstencil_format) {
        self.init()

        self.textureType = .type2D
        self.pixelFormat = colorFormat.mtlFormat
        self.width = Int(width)
        self.height = Int(height)
        self.depth = 1
        self.sampleCount = 1
        self.arrayLength = 1
        self.cpuCacheMode = .defaultCache
        self.allowGPUOptimizedContents = true
        self.hazardTrackingMode = .default
        self.mipmapLevelCount = 1
        self.storageMode = .private
        self.usage = [.shaderRead]
    }
}
