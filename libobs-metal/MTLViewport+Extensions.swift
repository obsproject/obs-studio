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

extension MTLViewport: @retroactive Equatable {
    /// Checks two ``MTLViewPort`` objects for equality
    /// - Parameters:
    ///   - lhs: First ``MTLViewPort``object
    ///   - rhs: Second ``MTLViewPort`` object
    /// - Returns: `true` if the dimensions and origins of both view ports match, `false` otherwise.
    public static func == (lhs: MTLViewport, rhs: MTLViewport) -> Bool {
        lhs.width == rhs.width && lhs.height == rhs.height && lhs.originX == rhs.originX
            && lhs.originY == rhs.originY
    }
}
