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

extension MTLTextureType {
    /// Converts the Metal texture type into a compatible `libobs` texture type or `nil` if no compatible mapping is
    /// possible.
    var gsTextureType: gs_texture_type? {
        switch self {
        case .type2D:
            return GS_TEXTURE_2D
        case .type3D:
            return GS_TEXTURE_3D
        case .typeCube:
            return GS_TEXTURE_CUBE
        default:
            return nil
        }
    }
}
