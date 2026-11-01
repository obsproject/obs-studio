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

import Foundation
import Metal

extension MTLTexture {
    /// Creates an opaque pointer of a ``MTLTexture`` instance and increases the reference count.
    /// - Returns: Opaque pointer for the ``MTLTexture``
    func getRetained() -> OpaquePointer {
        let retained = Unmanaged.passRetained(self).toOpaque()

        return OpaquePointer(retained)
    }

    /// Creates an opaque pointer of a ``MTLTexture`` instance without increasing the reference count.
    /// - Returns: Opaque pointer for the ``MTLTexture``
    func getUnretained() -> OpaquePointer {
        let unretained = Unmanaged.passUnretained(self).toOpaque()

        return OpaquePointer(unretained)
    }
}

extension MTLTexture {
    /// Convenience property to get the texture's size as a ``MTLSize`` object
    var size: MTLSize {
        .init(
            width: self.width,
            height: self.height,
            depth: self.depth
        )
    }

    /// Convenience property to get the texture's region as a ``MTLRegion`` object
    var region: MTLRegion {
        .init(
            origin: .init(x: 0, y: 0, z: 0),
            size: self.size
        )
    }

    /// Gets a new ``MTLTextureDescriptor`` instance with the properties of the texture
    var descriptor: MTLTextureDescriptor {
        let descriptor = MTLTextureDescriptor()

        descriptor.textureType = self.textureType
        descriptor.pixelFormat = self.pixelFormat
        descriptor.width = self.width
        descriptor.height = self.height
        descriptor.depth = self.depth
        descriptor.mipmapLevelCount = self.mipmapLevelCount
        descriptor.sampleCount = self.sampleCount
        descriptor.arrayLength = self.arrayLength
        descriptor.storageMode = self.storageMode
        descriptor.cpuCacheMode = self.cpuCacheMode
        descriptor.usage = self.usage
        descriptor.allowGPUOptimizedContents = self.allowGPUOptimizedContents

        return descriptor
    }
}
