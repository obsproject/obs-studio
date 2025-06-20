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
import Foundation
import Metal

private let bgraSurfaceFormat = kCVPixelFormatType_32BGRA  // 0x42_47_52_41
private let l10rSurfaceFormat = kCVPixelFormatType_ARGB2101010LEPacked  // 0x6C_31_30_72

enum MetalTextureMapMode {
    case unmapped
    case read
    case write
}

/// Struct used for data exchange between ``MetalTexture`` and `libobs` API functions during mapping and unmapping of
/// textures.
struct MetalTextureMapping {
    let mode: MetalTextureMapMode
    let rowSize: Int
    let data: UnsafeMutableRawPointer
}

/// Convenience class for managing ``MTLTexture`` objects
class MetalTexture {
    private let descriptor: MTLTextureDescriptor
    private var mappingMode: MetalTextureMapMode
    private let resourceID: UUID

    weak var device: MetalDevice?
    var data: UnsafeMutableRawPointer?
    var hasPendingWrites: Bool = false
    var sRGBtexture: MTLTexture?
    var texture: MTLTexture
    var stageBuffer: MetalStageBuffer?

    /// Binds the provided `IOSurfaceRef` to a new `MTLTexture` instance
    /// - Parameters:
    ///   - device: `MTLDevice` instance to use for texture object creation
    ///   - surface: `IOSurfaceRef` reference to an existing `IOSurface`
    /// - Returns: `MTLTexture` instance if texture was created successfully, `nil` otherwise
    private static func bindSurface(device: MetalDevice, surface: IOSurfaceRef) -> MTLTexture? {
        guard let pixelFormat = MTLPixelFormat.init(osType: IOSurfaceGetPixelFormat(surface)) else {
            assertionFailure("MetalDevice: IOSurface pixel format is not supported")
            return nil
        }

        let descriptor = MTLTextureDescriptor.texture2DDescriptor(
            pixelFormat: pixelFormat,
            width: IOSurfaceGetWidth(surface),
            height: IOSurfaceGetHeight(surface),
            mipmapped: false
        )

        descriptor.usage = [.shaderRead]

        let texture = device.device.makeTexture(descriptor: descriptor, iosurface: surface, plane: 0)
        return texture
    }

    /// Creates a new ``MetalDevice`` instance with the provided `MTLTextureDescriptor`
    /// - Parameters:
    ///   - device: `MTLDevice` instance to use for texture object creation
    ///   - descriptor: `MTLTextureDescriptor` to use for texture object creation
    init?(device: MetalDevice, descriptor: MTLTextureDescriptor) {
        self.device = device

        let texture = device.device.makeTexture(descriptor: descriptor)

        guard let texture else {
            assertionFailure(
                "MetalTexture: Failed to create texture with size \(descriptor.width)x\(descriptor.height)")
            return nil
        }

        self.texture = texture

        self.resourceID = UUID()
        self.mappingMode = .unmapped
        self.descriptor = texture.descriptor

        updateSRGBView()
    }

    /// Creates a new ``MetalDevice`` instance with the provided `IOSurfaceRef`
    /// - Parameters:
    ///   - device: `MTLDevice` instance to use for texture object creation
    ///   - surface: `IOSurfaceRef` to use for texture object creation
    init?(device: MetalDevice, surface: IOSurfaceRef) {
        self.device = device

        let texture = MetalTexture.bindSurface(device: device, surface: surface)

        guard let texture else {
            assertionFailure("MetalTexture: Failed to create texture with IOSurface")
            return nil
        }

        self.texture = texture

        self.resourceID = UUID()
        self.mappingMode = .unmapped
        self.descriptor = texture.descriptor

        updateSRGBView()
    }

    /// Creates a new ``MetalDevice`` instance with the provided `MTLTexture`
    /// - Parameters:
    ///   - device: `MTLDevice` instance to use for future texture operations
    ///   - surface: `MTLTexture` to wrap in the ``MetalDevice`` instance
    init?(device: MetalDevice, texture: MTLTexture) {
        self.device = device
        self.texture = texture

        self.resourceID = UUID()
        self.mappingMode = .unmapped
        self.descriptor = texture.descriptor

        updateSRGBView()
    }

    /// Creates a new ``MetalDevice`` instance with a placeholder texture
    /// - Parameters:
    ///   - device: `MTLDevice` instance to use for future texture operations
    ///
    /// This constructor creates a "placeholder" object that can be shared with `libobs` or updated with an actual
    /// `MTLTexture` later.
    init?(device: MetalDevice) {
        self.device = device

        let descriptor = MTLTextureDescriptor.texture2DDescriptor(
            pixelFormat: .bgra8Unorm, width: 2, height: 2, mipmapped: false)

        guard let texture = device.device.makeTexture(descriptor: descriptor) else {
            assertionFailure("MetalTexture: Failed to create placeholder texture object")
            return nil
        }

        self.texture = texture
        self.sRGBtexture = nil
        self.resourceID = UUID()
        self.mappingMode = .unmapped
        self.descriptor = texture.descriptor
    }

    /// Updates the ``MetalTexture`` with a new `IOSurfaceRef`
    /// - Parameter surface: Updated `IOSurfaceRef` to a new `IOSurface`
    /// - Returns: `true` if update was successful, `false` otherwise
    ///
    /// "Rebinding" was used with the OpenGL backend, but is not available in Metal. Instead a new `MTLTexture` is
    /// created with the provided `IOSurfaceRef` and the ``MetalTexture`` is updated accordingly.
    ///
    func rebind(surface: IOSurfaceRef) -> Bool {
        guard let device = self.device, let texture = MetalTexture.bindSurface(device: device, surface: surface) else {
            assertionFailure("MetalTexture: Failed to rebind IOSurface to texture")
            return false
        }

        self.texture = texture

        updateSRGBView()

        return true
    }

    /// Creates a `MTLTextureView` for the texture wrapped by the ``MetalTexture`` instance with a corresponding sRGB
    /// pixel format, if the texture's pixel format has an appropriate sRGB variant.
    func updateSRGBView() {
        guard !texture.isFramebufferOnly else {
            self.sRGBtexture = nil
            return
        }

        let sRGBFormat: MTLPixelFormat? =
            switch texture.pixelFormat {
            case .bgra8Unorm: .bgra8Unorm_srgb
            case .rgba8Unorm: .rgba8Unorm_srgb
            case .r8Unorm: .r8Unorm_srgb
            case .rg8Unorm: .rg8Unorm_srgb
            case .bgra10_xr: .bgra10_xr_srgb
            default: nil
            }

        if let sRGBFormat {
            self.sRGBtexture = texture.makeTextureView(pixelFormat: sRGBFormat)
        } else {
            self.sRGBtexture = nil
        }
    }

    /// Downloads pixel data from the wrapped `MTLTexture` to the memory location provided by a pointer.
    /// - Parameters:
    ///   - data: Pointer to memory that should receive the texture data
    ///   - mipmapLevel: Mipmap level of the texture to copy data from
    ///
    /// > Important: The access of texture data is neither protected nor synchronized. If any draw calls to the texture
    /// take place while this function is executed, the downloaded data will reflect this. Use explicit synchronization
    /// before initiating a download to prevent this.
    func download(data: UnsafeMutableRawPointer, mipmapLevel: Int = 0) {
        let mipmapWidth = texture.width >> mipmapLevel
        let mipmapHeight = texture.height >> mipmapLevel

        let rowSize = mipmapWidth * texture.pixelFormat.bytesPerPixel!
        let region = MTLRegionMake2D(0, 0, mipmapWidth, mipmapHeight)

        texture.getBytes(data, bytesPerRow: rowSize, from: region, mipmapLevel: mipmapLevel)
    }

    /// Uploads pixel data into the  wrappred `MTLTexture` from the memory location provided by a pointer.
    /// - Parameters:
    ///   - data: Pointer to memory that contains the texture data
    ///   - mipmapLevels: Mipmap level of the texture to copy data into
    ///
    /// > Important: The write access of texture data is neither protected nor synchronized. If any draw calls use this
    /// texture for reading or writing while this function is executed, the upload might have been incomplete or the
    /// data might have been overwritten by the GPU. Use explicit synchronization before initiaitng an upload to
    /// prevent this.
    func upload(data: UnsafePointer<UnsafePointer<UInt8>?>, mipmapLevels: Int) {
        let bytesPerPixel = texture.pixelFormat.bytesPerPixel!

        switch texture.textureType {
        case .type2D, .typeCube:
            let textureCount = if texture.textureType == .typeCube { 6 } else { 1 }

            let data = UnsafeBufferPointer(start: data, count: (textureCount * mipmapLevels))

            for i in 0..<textureCount {
                for mipmapLevel in 0..<mipmapLevels {
                    let index = mipmapLevels * i + mipmapLevel

                    guard let data = data[index] else { break }

                    let mipmapWidth = texture.width >> mipmapLevel
                    let mipmapHeight = texture.height >> mipmapLevel
                    let rowSize = mipmapWidth * bytesPerPixel

                    let region = MTLRegionMake2D(0, 0, mipmapWidth, mipmapHeight)

                    texture.replace(
                        region: region, mipmapLevel: mipmapLevel, slice: i, withBytes: data, bytesPerRow: rowSize,
                        bytesPerImage: 0)
                }
            }
        case .type3D:
            let data = UnsafeBufferPointer(start: data, count: mipmapLevels)

            for (mipmapLevel, mipmapData) in data.enumerated() {
                guard let mipmapData else { break }

                let mipmapWidth = texture.width >> mipmapLevel
                let mipmapHeight = texture.height >> mipmapLevel
                let mipmapDepth = texture.depth >> mipmapLevel
                let rowSize = mipmapWidth * bytesPerPixel
                let imageSize = rowSize * mipmapHeight

                let region = MTLRegionMake3D(0, 0, 0, mipmapWidth, mipmapHeight, mipmapDepth)

                texture.replace(
                    region: region,
                    mipmapLevel: mipmapLevel,
                    slice: 0,
                    withBytes: mipmapData,
                    bytesPerRow: rowSize,
                    bytesPerImage: imageSize
                )
            }
        default:
            fatalError("MetalTexture: Unsupported texture type \(texture.textureType)")
        }

        if texture.mipmapLevelCount > 1 {
            let device = self.device!

            try? device.ensureCommandBuffer()

            guard let buffer = device.renderState.commandBuffer,
                let encoder = buffer.makeBlitCommandEncoder()
            else {
                assertionFailure("MetalTexture: Failed to create command buffer for mipmap generation")
                return
            }

            encoder.generateMipmaps(for: texture)
            encoder.endEncoding()
        }
    }

    /// Emulates the "map" operation available in Direct3D, providing a pointer for texture uploads or downloads
    /// - Parameters:
    ///   - mode: Map mode to use (writing or reading)
    ///   - mipmapLevel: Mip map level to map
    /// - Returns: A ``MetalTextureMapping`` struct that provides the result of the mapping
    ///
    /// In Direct3D a "map" operation will do many things at once depending on the current state of its pipelines and
    /// the mapping mode used:
    /// * When mapped for writing, Direct3D will provide a pointer to CPU memory into which an application can write
    ///   new texture data.
    /// * When mapped for reading, Direct3D will provide a pointer to CPU memory into which it has copied the contents
    ///   of the texture
    ///
    /// In either case, the texture will be blocked from access by the GPU until it is unmapped again. In some cases a
    /// "map" operation will also implicitly initiate a "flush" operation to ensure that pending GPU commands involving
    /// this texture are submitted before it becomes unavailable.
    ///
    /// Metal does not provide such a convenience method and because `libobs` operates under the assumption that it has
    /// to copy its own data into a memory location provided by Direct3D, this has to be emulated explicitly here,
    /// albeit without the blocking of access to the texture.
    ///
    /// This function always needs to be balanced by an appropriate ``unmap`` call.
    func map(mode: MetalTextureMapMode, mipmapLevel: Int = 0) -> MetalTextureMapping? {
        guard mappingMode == .unmapped else {
            assertionFailure("MetalTexture: Attempted to map already-mapped texture.")
            return nil
        }

        let mipmapWidth = texture.width >> mipmapLevel
        let mipmapHeight = texture.height >> mipmapLevel

        let rowSize = mipmapWidth * texture.pixelFormat.bytesPerPixel!
        let dataSize = rowSize * mipmapHeight

        // TODO: Evaluate whether a blit to/from a `MTLBuffer` with its `contents` pointer shared is more efficient
        let data = UnsafeMutableRawBufferPointer.allocate(byteCount: dataSize, alignment: MemoryLayout<UInt8>.alignment)

        guard let baseAddress = data.baseAddress else {
            return nil
        }

        if mode == .read {
            download(data: baseAddress, mipmapLevel: mipmapLevel)
        }

        self.data = baseAddress
        self.mappingMode = mode

        let mapping = MetalTextureMapping(
            mode: mode,
            rowSize: rowSize,
            data: baseAddress
        )

        return mapping
    }

    /// Emulates the "unmap" operation available in Direct3D
    /// - Parameter mipmapLevel: The mipmap level that is to be unmapped
    ///
    /// This function will replace the contents of the "mapped" texture with the data written into the memory provided
    /// by the "mapping".
    ///
    /// As such this function has to always balance the corresponding ``map`` call to ensure that the data written into
    /// the provided memory location is written into the texture and the memory itself is deallocated.
    func unmap(mipmapLevel: Int = 0) {
        guard mappingMode != .unmapped else {
            assertionFailure("MetalTexture: Attempted to unmap an unmapped texture")
            return
        }

        let mipmapWidth = texture.width >> mipmapLevel
        let mipmapHeight = texture.height >> mipmapLevel

        let rowSize = mipmapWidth * texture.pixelFormat.bytesPerPixel!
        let region = MTLRegionMake2D(0, 0, mipmapWidth, mipmapHeight)

        if let textureData = self.data {
            if self.mappingMode == .write {
                texture.replace(
                    region: region,
                    mipmapLevel: mipmapLevel,
                    withBytes: textureData,
                    bytesPerRow: rowSize
                )
            }

            textureData.deallocate()
            self.data = nil
        }

        self.mappingMode = .unmapped
    }

    /// Gets an opaque pointer for the ``MetalTexture`` instance and increases its reference count by one
    /// - Returns: `OpaquePointer` to class instance
    ///
    /// > Note: Use this method when the instance is to be shared via an `OpaquePointer` and needs to be retained. Any
    /// opaque pointer shared this way  needs to be converted into a retained reference again to ensure automatic
    /// deinitialization by the Swift runtime.
    func getRetained() -> OpaquePointer {
        let retained = Unmanaged.passRetained(self).toOpaque()

        return OpaquePointer(retained)
    }

    /// Gets an opaque pointer for the ``MetalTexture`` instance without increasing its reference count
    /// - Returns: `OpaquePointer` to class instance
    func getUnretained() -> OpaquePointer {
        let unretained = Unmanaged.passUnretained(self).toOpaque()

        return OpaquePointer(unretained)
    }
}

/// Extends the ``MetalTexture`` class with comparison operators and a hash function to enable the use inside a `Set`
/// collection
extension MetalTexture: Hashable {
    static func == (lhs: MetalTexture, rhs: MetalTexture) -> Bool {
        lhs.resourceID == rhs.resourceID
    }

    static func != (lhs: MetalTexture, rhs: MetalTexture) -> Bool {
        lhs.resourceID != rhs.resourceID
    }

    func hash(into hasher: inout Hasher) {
        hasher.combine(resourceID)
    }
}
