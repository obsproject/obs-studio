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

import CoreMediaIO

enum CMIOPropertyType {
    case string
    case objectId, streamId
    indirect case array(CMIOPropertyType)
}

enum CMIOPropertyValue {
    case string(String)
    case objectId(CMIOObjectID)
    case streamId(CMIOStreamID)
    case objectIds([CMIOObjectID])
    case streamIds([CMIOStreamID])
}

extension CMIOObjectPropertyElement {
    static let main = CMIOObjectPropertyElement(kCMIOObjectPropertyElementMain)
}

extension CMIOObjectPropertyScope {
    static let global = CMIOObjectPropertyScope(kCMIOObjectPropertyScopeGlobal)
}

extension CMIOObjectID {
    static let system = CMIOObjectID(kCMIOObjectSystemObject)
    static let unknown = CMIOObjectID(kCMIOObjectUnknown)
}

private func getCMIOPropertyFor<T>(
    selector: CMIOObjectPropertySelector, scope: CMIOObjectPropertyScope, element: CMIOObjectPropertyElement,
    in objectId: CMIOObjectID
) -> T? {
    var address = CMIOObjectPropertyAddress(
        mSelector: selector,
        mScope: scope,
        mElement: element
    )

    let dataSize: UInt32 = UInt32(MemoryLayout<T>.size)
    var dataUsed: UInt32 = 0

    let opaqueData = UnsafeMutableRawPointer.allocate(byteCount: Int(dataSize), alignment: MemoryLayout<T>.alignment)

    defer { opaqueData.deallocate() }

    let status = CMIOObjectGetPropertyData(objectId, &address, 0, nil, dataSize, &dataUsed, opaqueData)

    guard status == kCMIOHardwareNoError else {
        return nil
    }

    let typedData = opaqueData.bindMemory(to: T.self, capacity: 1)

    return typedData.pointee
}

private func getCMIOPropertyArrayFor<T>(
    selector: CMIOObjectPropertySelector, scope: CMIOObjectPropertyScope, element: CMIOObjectPropertyElement,
    in objectId: CMIOObjectID
) -> [T]? {
    var address = CMIOObjectPropertyAddress(
        mSelector: selector,
        mScope: scope,
        mElement: element
    )

    var dataSize: UInt32 = 0
    var dataUsed: UInt32 = 0

    var status = CMIOObjectGetPropertyDataSize(objectId, &address, 0, nil, &dataSize)

    guard status == kCMIOHardwareNoError else {
        return nil
    }

    let count = Int(dataSize) / MemoryLayout<T>.size

    let opaqueData = UnsafeMutableRawPointer.allocate(byteCount: Int(dataSize), alignment: MemoryLayout<T>.alignment)

    defer { opaqueData.deallocate() }

    status = CMIOObjectGetPropertyData(objectId, &address, 0, nil, dataSize, &dataUsed, opaqueData)

    guard status == kCMIOHardwareNoError else {
        return nil
    }

    let typedData = opaqueData.bindMemory(to: T.self, capacity: count)
    return UnsafeBufferPointer<T>(start: typedData, count: count).map { $0 }
}

struct CMIOObjectProperty {
    let selector: CMIOObjectPropertySelector
    let type: CMIOPropertyType

    init(selector: Int, type: CMIOPropertyType) {
        self.selector = CMIOObjectPropertySelector(selector)
        self.type = type
    }

    func hasProperty(
        scope: CMIOObjectPropertyScope, element: CMIOObjectPropertyElement,
        in objectId: CMIOObjectID
    ) -> Bool {
        var address = CMIOObjectPropertyAddress(mSelector: selector, mScope: scope, mElement: element)

        return CMIOObjectHasProperty(objectId, &address)
    }

    func get(
        scope: CMIOObjectPropertyScope, element: CMIOObjectPropertyElement,
        in objectId: CMIOObjectID
    ) -> CMIOPropertyValue? {
        switch type {
        case .string:
            if let value: CFString = getCMIOPropertyFor(
                selector: selector, scope: scope, element: element, in: objectId)
            {
                return .string(value as String)
            }
        case .objectId:
            if let value: CMIOObjectID = getCMIOPropertyFor(
                selector: selector, scope: scope, element: element, in: objectId)
            {
                return .objectId(value)
            }
        case .streamId:
            if let value: CMIOStreamID = getCMIOPropertyFor(
                selector: selector, scope: scope, element: element, in: objectId)
            {
                return .streamId(value)
            }
        case .array(let elementType):
            switch elementType {
            case .objectId:
                if let value: [CMIOObjectID] = getCMIOPropertyArrayFor(
                    selector: selector, scope: scope, element: element, in: objectId)
                {
                    return .objectIds(value)
                }
            case .streamId:
                if let value: [CMIOStreamID] = getCMIOPropertyArrayFor(
                    selector: selector, scope: scope, element: element, in: objectId)
                {
                    return .streamIds(value)
                }
            default:
                assertionFailure("Unsupported CMIOObjectProperty type: \(elementType)")
            }
        }

        return nil
    }
}
