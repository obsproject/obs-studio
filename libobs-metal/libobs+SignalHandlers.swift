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

enum MetalSignalType: String {
    case videoReset = "video_reset"
}

/// Dispatches the video reset event to the ``MetalDevice`` instance
/// - Parameters:
///   - param: Opaque pointer to a ``MetalDevice`` instance
///   - _: Unused pointer to signal callback data
public func metal_video_reset_handler(_ param: UnsafeMutableRawPointer?, _: UnsafeMutablePointer<calldata>?) {
    guard let param else { return }

    let metalDevice = unsafeBitCast(param, to: MetalDevice.self)

    metalDevice.dispatchSignal(type: .videoReset)
}
