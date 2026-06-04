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

/// Creates a new ``MTLSamplerDescriptor`` to share as an opaque pointer with `libobs`
/// - Parameters:
///   - device: Opaque pointer to ``MetalDevice`` instance shared with `libobs`
///   - info: Sampler information encoded as a `gs_sampler_info` struct
/// - Returns: Opaque pointer to a new ``MTLSamplerDescriptor`` instance on success, `nil` otherwise
@_cdecl("device_samplerstate_create")
public func device_samplerstate_create(device: UnsafeRawPointer, info: gs_sampler_info) -> OpaquePointer? {
    let device: MetalDevice = unretained(device)

    guard let sAddressMode = info.address_u.mtlMode,
        let tAddressMode = info.address_v.mtlMode,
        let rAddressMode = info.address_w.mtlMode
    else {
        assertionFailure("device_samplerstate_create: Invalid address modes provided")
        return nil
    }

    guard let minFilter = info.filter.minMagFilter, let magFilter = info.filter.minMagFilter,
        let mipFilter = info.filter.mipFilter
    else {
        assertionFailure("device_samplerstate_create: Invalid filter modes provided")
        return nil
    }

    let descriptor = MTLSamplerDescriptor()
    descriptor.sAddressMode = sAddressMode
    descriptor.tAddressMode = tAddressMode
    descriptor.rAddressMode = rAddressMode

    descriptor.minFilter = minFilter
    descriptor.magFilter = magFilter
    descriptor.mipFilter = mipFilter

    descriptor.maxAnisotropy = max(16, min(1, Int(info.max_anisotropy)))

    descriptor.compareFunction = .always
    descriptor.borderColor =
        if (info.border_color & 0x00_00_00_FF) == 0 {
            .transparentBlack
        } else if info.border_color == 0xFF_FF_FF_FF {
            .opaqueWhite
        } else {
            .opaqueBlack
        }

    guard let samplerState = device.device.makeSamplerState(descriptor: descriptor) else {
        assertionFailure("device_samplerstate_create: Unable to create sampler state")
        return nil
    }

    let retained = Unmanaged.passRetained(samplerState).toOpaque()

    return OpaquePointer(retained)
}

/// Requests the deinitialization of the ``MTLSamplerState`` instance shared with `libobs`
/// - Parameter samplerstate: Opaque pointer to ``MTLSamplerState`` instance shared with `libobs`
///
/// Ownership of the ``MTLSamplerState`` instance will be transferred into the function and if this was the last
/// strong reference to it, the object will be automatically deinitialized and deallocated by Swift.
@_cdecl("gs_samplerstate_destroy")
public func gs_samplerstate_destroy(samplerstate: UnsafeRawPointer) {
    let _ = retained(samplerstate) as MTLSamplerState
}

/// Loads the provided ``MTLSamplerState`` into the current pipeline's sampler array at the requested texture unit
/// number
/// - Parameters:
///   - device: Opaque pointer to ``MetalDevice`` instance shared with `libobs`
///   - samplerstate: Opaque pointer to ``MTLSamplerState`` instance shared with `libobs`
///   - unit: Number identifying the "texture slot" used by OBS Studio's renderer.
///
///   Texture slot numbers are equivalent to array index and represent a direct mapping between samplers and textures.
@_cdecl("device_load_samplerstate")
public func device_load_samplerstate(device: UnsafeRawPointer, samplerstate: UnsafeRawPointer, unit: UInt32) {
    let device: MetalDevice = unretained(device)
    let samplerState: MTLSamplerState = unretained(samplerstate)

    device.renderState.samplers[Int(unit)] = samplerState
}
