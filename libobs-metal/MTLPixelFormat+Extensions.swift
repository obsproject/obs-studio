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

import CoreGraphics
import CoreVideo
import Foundation
import Metal

extension MTLPixelFormat {
    /// Property to check whether the pixel format is an 8-bit format
    var is8Bit: Bool {
        switch self {
        case .a8Unorm, .r8Unorm, .r8Snorm, .r8Uint, .r8Sint:
            return true
        case .r8Unorm_srgb:
            return true
        default:
            return false
        }
    }

    /// Property to check whether the pixel format is a 16-bit format
    var is16Bit: Bool {
        switch self {
        case .r16Unorm, .r16Snorm, .r16Uint, .r16Sint:
            return true
        case .rg8Unorm, .rg8Snorm, .rg8Uint, .rg8Sint:
            return true
        case .rg16Float:
            return true
        case .rg8Unorm_srgb:
            return true
        default:
            return false
        }
    }

    /// Property to check whether the pixel format is a packed 16-bit format
    var isPacked16Bit: Bool {
        switch self {
        case .b5g6r5Unorm, .a1bgr5Unorm, .abgr4Unorm, .bgr5A1Unorm:
            return true
        default:
            return false
        }
    }

    /// Property to check whether the pixel format is a 32-bit format
    var is32Bit: Bool {
        switch self {
        case .r32Uint, .r32Sint:
            return true
        case .r32Float:
            return true
        case .rg16Unorm, .rg16Snorm, .rg16Uint, .rg16Sint:
            return true
        case .rg16Float:
            return true
        case .rgba8Unorm, .rgba8Snorm, .rgba8Uint, .rgba8Sint, .bgra8Unorm:
            return true
        case .rgba8Unorm_srgb, .bgra8Unorm_srgb:
            return true
        default:
            return false
        }
    }

    /// Property to check whether the pixel format is a packed 32-bit format
    var isPacked32Bit: Bool {
        switch self {
        case .rgb10a2Unorm, .rgb10a2Uint, .bgr10a2Unorm:
            return true
        case .rg11b10Float:
            return true
        case .rgb9e5Float:
            return true
        case .bgr10_xr, .bgr10_xr_srgb:
            return true
        default:
            return false
        }
    }

    /// Property to check whether the pixel format is a 64-bit format
    var is64Bit: Bool {
        switch self {
        case .rg32Uint, .rg32Sint:
            return true
        case .rg32Float:
            return true
        case .rgba16Unorm, .rgba16Snorm, .rgba16Uint, .rgba16Sint:
            return true
        case .rgba16Float:
            return true
        case .bgra10_xr, .bgra10_xr_srgb:
            return true
        default:
            return false
        }
    }

    /// Property to check whether the pixel format is a 128-bit format
    var is128Bit: Bool {
        switch self {
        case .rgba32Uint, .rgba32Sint:
            return true
        case .rgba32Float:
            return true
        default:
            return false
        }
    }

    /// Property to check whether the pixel format will trigger automatic sRGB gamma encoding and decoding
    var isSRGB: Bool {
        switch self {
        case .r8Unorm_srgb, .rg8Unorm_srgb, .bgra8Unorm_srgb, .rgba8Unorm_srgb:
            return true
        case .bgr10_xr_srgb, .bgra10_xr_srgb:
            return true
        case .astc_4x4_srgb, .astc_5x4_srgb, .astc_5x5_srgb, .astc_6x5_srgb, .astc_6x6_srgb, .astc_8x5_srgb,
            .astc_8x6_srgb, .astc_8x8_srgb, .astc_10x5_srgb, .astc_10x6_srgb, .astc_10x8_srgb, .astc_10x10_srgb,
            .astc_12x10_srgb, .astc_12x12_srgb:
            return true
        case .bc1_rgba_srgb, .bc2_rgba_srgb, .bc3_rgba_srgb, .bc7_rgbaUnorm_srgb:
            return true
        case .eac_rgba8_srgb, .etc2_rgb8, .etc2_rgb8a1_srgb:
            return true
        default:
            return false
        }
    }

    /// Property to check whether the pixel format is an extended dynamic range (EDR) format
    var isEDR: Bool {
        switch self {
        case .bgr10_xr, .bgra10_xr, .bgr10_xr_srgb, .bgra10_xr_srgb:
            return true
        default:
            return false
        }
    }

    /// Property to check whether the pixel format uses a form of texture compression
    var isCompressed: Bool {
        switch self {
        // S3TC
        case .bc1_rgba, .bc1_rgba_srgb, .bc2_rgba, .bc2_rgba_srgb, .bc3_rgba, .bc3_rgba_srgb:
            return true
        // RGTC
        case .bc4_rUnorm, .bc4_rSnorm, .bc5_rgUnorm, .bc5_rgSnorm:
            return true
        // BPTC
        case .bc6H_rgbFloat, .bc6H_rgbuFloat, .bc7_rgbaUnorm, .bc7_rgbaUnorm_srgb:
            return true
        // EAC
        case .eac_r11Unorm, .eac_r11Snorm, .eac_rg11Unorm, .eac_rg11Snorm, .eac_rgba8, .eac_rgba8_srgb:
            return true
        // ETC
        case .etc2_rgb8, .etc2_rgb8_srgb, .etc2_rgb8a1, .etc2_rgb8a1_srgb:
            return true
        // ASTC
        case .astc_4x4_srgb, .astc_5x4_srgb, .astc_5x5_srgb, .astc_6x5_srgb, .astc_6x6_srgb, .astc_8x5_srgb,
            .astc_8x6_srgb, .astc_8x8_srgb, .astc_10x5_srgb, .astc_10x6_srgb, .astc_10x8_srgb, .astc_10x10_srgb,
            .astc_12x10_srgb, .astc_12x12_srgb, .astc_4x4_ldr, .astc_5x4_ldr, .astc_5x5_ldr, .astc_6x5_ldr,
            .astc_6x6_ldr, .astc_8x5_ldr, .astc_8x6_ldr, .astc_8x8_ldr, .astc_10x5_ldr, .astc_10x6_ldr, .astc_10x8_ldr,
            .astc_10x10_ldr, .astc_12x10_ldr, .astc_12x12_ldr:
            return true
        // ASTC HDR
        case .astc_4x4_hdr, .astc_5x4_hdr, .astc_5x5_hdr, .astc_6x5_hdr, .astc_6x6_hdr, .astc_8x5_hdr, .astc_8x6_hdr,
            .astc_8x8_hdr, .astc_10x5_hdr, .astc_10x6_hdr, .astc_10x8_hdr, .astc_10x10_hdr, .astc_12x10_hdr,
            .astc_12x12_hdr:
            return true
        default:
            return false
        }
    }

    /// Property to check whether the pixel format is a depth buffer format
    var isDepth: Bool {
        switch self {
        case .depth16Unorm, .depth32Float:
            return true
        default:
            return false
        }
    }

    /// Property to check whether the pixel format is depth stencil format
    var isStencil: Bool {
        switch self {
        case .stencil8, .x24_stencil8, .x32_stencil8, .depth24Unorm_stencil8, .depth32Float_stencil8:
            return true
        default:
            return false
        }
    }

    /// Returns number of color components used by the pixel format
    var componentCount: Int? {
        switch self {
        case .a8Unorm, .r8Unorm, .r8Snorm, .r8Uint, .r8Sint, .r8Unorm_srgb:
            return 1
        case .r16Unorm, .r16Snorm, .r16Uint, .r16Sint, .r16Float:
            return 1
        case .r32Uint, .r32Sint, .r32Float:
            return 1
        case .rg8Unorm, .rg8Snorm, .rg8Uint, .rg8Sint, .rg8Unorm_srgb:
            return 2
        case .rg16Unorm, .rg16Snorm, .rg16Uint, .rg16Sint:
            return 2
        case .rg32Uint, .rg32Sint, .rg32Float:
            return 2
        case .b5g6r5Unorm, .rg11b10Float, .rgb9e5Float, .gbgr422, .bgrg422:
            return 3
        case .a1bgr5Unorm, .abgr4Unorm, .bgr5A1Unorm:
            return 4
        case .rgba8Unorm, .rgba8Snorm, .rgba8Uint, .rgba8Sint, .rgba8Unorm_srgb, .bgra8Unorm, .bgra8Unorm_srgb:
            return 4
        case .rgb10a2Unorm, .rgb10a2Uint, .bgr10a2Unorm, .bgr10_xr, .bgr10_xr_srgb:
            return 4
        case .rgba16Unorm, .rgba16Snorm, .rgba16Uint, .rgba16Sint, .rgba16Float:
            return 4
        case .rgba32Uint, .rgba32Sint, .rgba32Float:
            return 4
        case .bc4_rUnorm, .bc4_rSnorm, .eac_r11Unorm, .eac_r11Snorm:
            return 1
        case .bc5_rgUnorm, .bc5_rgSnorm:
            return 2
        case .bc6H_rgbFloat, .bc6H_rgbuFloat, .eac_rg11Unorm, .eac_rg11Snorm, .etc2_rgb8, .etc2_rgb8_srgb:
            return 3
        case .bc1_rgba, .bc1_rgba_srgb, .bc2_rgba, .bc2_rgba_srgb, .bc3_rgba, .bc3_rgba_srgb, .etc2_rgb8a1,
            .etc2_rgb8a1_srgb, .eac_rgba8, .eac_rgba8_srgb, .bc7_rgbaUnorm, .bc7_rgbaUnorm_srgb:
            return 4
        default:
            return nil
        }
    }

    /// Conversion of pixel format to `libobs` color format
    var gsColorFormat: gs_color_format {
        switch self {
        case .a8Unorm:
            return GS_A8
        case .r8Unorm:
            return GS_R8
        case .rgba8Unorm:
            return GS_RGBA
        case .bgra8Unorm:
            return GS_BGRA
        case .rgb10a2Unorm:
            return GS_R10G10B10A2
        case .rgba16Unorm:
            return GS_RGBA16
        case .r16Unorm:
            return GS_R16
        case .rgba16Float:
            return GS_RGBA16F
        case .rgba32Float:
            return GS_RGBA32F
        case .rg16Float:
            return GS_RG16F
        case .rg32Float:
            return GS_RG32F
        case .r16Float:
            return GS_R16F
        case .r32Float:
            return GS_R32F
        case .bc1_rgba:
            return GS_DXT1
        case .bc2_rgba:
            return GS_DXT3
        case .bc3_rgba:
            return GS_DXT5
        default:
            return GS_UNKNOWN
        }
    }

    /// Returns the bits per pixel based on the pixel format
    var bitsPerPixel: Int? {
        if self.is8Bit {
            return 8
        } else if self.is16Bit || self.isPacked16Bit {
            return 16
        } else if self.is32Bit || self.isPacked32Bit {
            return 32
        } else if self.is64Bit {
            return 64
        } else if self.is128Bit {
            return 128
        } else {
            return nil
        }
    }

    /// Returns the bytes per pixel based on the pixel format
    var bytesPerPixel: Int? {
        if self.is8Bit {
            return 1
        } else if self.is16Bit || self.isPacked16Bit {
            return 2
        } else if self.is32Bit {
            return 4
        } else if self.isPacked32Bit {
            switch self {
            case .rgb10a2Unorm, .rgb10a2Uint, .bgr10a2Unorm, .rg11b10Float, .rgb9e5Float:
                return 4
            case .bgr10_xr, .bgr10_xr_srgb:
                return 8
            default:
                return nil
            }
        } else if self.is64Bit {
            return 8
        } else {
            return nil
        }
    }

    /// Returns the bytes used per color component of the pixel format
    var bitsPerComponent: Int? {
        if !self.isCompressed {
            if let bitsPerPixel = self.bitsPerPixel, let componentCount = self.componentCount {
                return bitsPerPixel / componentCount
            }
        }

        return nil
    }
}

extension MTLPixelFormat {
    /// Converts the pixel format into a compatible CoreGraphics color space
    var colorSpace: CGColorSpace? {
        switch self {
        case .a8Unorm, .r8Unorm, .r8Snorm, .r8Uint, .r8Sint, .r16Unorm, .r16Snorm, .r16Uint, .r16Sint,
            .r16Float, .r32Uint, .r32Sint, .r32Float:
            return CGColorSpace(name: CGColorSpace.linearGray)
        case .rg8Unorm, .rg8Snorm, .rg8Uint, .rg8Sint, .rgba8Unorm, .rgba8Snorm, .rgba8Uint, .rgba8Sint, .bgra8Unorm,
            .rgba16Unorm, .rgba16Snorm, .rgba16Uint, .rgba16Sint:
            return CGColorSpace(name: CGColorSpace.linearSRGB)
        case .rg8Unorm_srgb, .rgba8Unorm_srgb, .bgra8Unorm_srgb:
            return CGColorSpace(name: CGColorSpace.sRGB)
        case .rg16Float, .rg32Float, .rgba16Float, .rgba32Float, .bgr10_xr, .bgr10a2Unorm:
            return CGColorSpace(name: CGColorSpace.extendedLinearSRGB)
        case .bgr10_xr_srgb:
            return CGColorSpace(name: CGColorSpace.extendedSRGB)
        default:
            return nil
        }
    }
}

extension MTLPixelFormat {
    /// Initializes a ``MTLPixelFormat`` with a compatible CoreVideo video pixel format
    init?(osType: OSType) {
        guard let pixelFormat = osType.mtlFormat else {
            return nil
        }

        self = pixelFormat
    }

    /// Conversion of the pixel format into a compatible CoreVideo video pixel format
    var videoPixelFormat: OSType? {
        switch self {
        case .r8Unorm, .r8Unorm_srgb:
            return kCVPixelFormatType_OneComponent8
        case .r16Float:
            return kCVPixelFormatType_OneComponent16Half
        case .r32Float:
            return kCVPixelFormatType_OneComponent32Float
        case .rg8Unorm, .rg8Unorm_srgb:
            return kCVPixelFormatType_TwoComponent8
        case .rg16Float:
            return kCVPixelFormatType_TwoComponent16Half
        case .rg32Float:
            return kCVPixelFormatType_TwoComponent32Float
        case .bgra8Unorm, .bgra8Unorm_srgb:
            return kCVPixelFormatType_32BGRA
        case .rgba8Unorm, .rgba8Unorm_srgb:
            return kCVPixelFormatType_32RGBA
        case .rgba16Float:
            return kCVPixelFormatType_64RGBAHalf
        case .rgba32Float:
            return kCVPixelFormatType_128RGBAFloat
        default:
            return nil
        }
    }
}
