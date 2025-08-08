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

import Foundation

public enum OBSLogLevel: Int32 {
    case error = 100
    case warning = 200
    case info = 300
    case debug = 400
}

@inlinable
public func unretained<Instance>(_ pointer: UnsafeRawPointer) -> Instance where Instance: AnyObject {
    Unmanaged<Instance>.fromOpaque(pointer).takeUnretainedValue()
}

@inlinable
public func retained<Instance>(_ pointer: UnsafeRawPointer) -> Instance where Instance: AnyObject {
    Unmanaged<Instance>.fromOpaque(pointer).takeRetainedValue()
}

@inlinable
public func OBSLog(_ level: OBSLogLevel, _ format: String, _ args: CVarArg...) {
    let logMessage = String.localizedStringWithFormat(format, args)

    logMessage.withCString { cMessage in
        withVaList([cMessage]) { arguments in
            blogva(level.rawValue, "[mac-virtualcam] %s", arguments)
        }
    }
}

// MARK: - OBS Module API

nonisolated(unsafe) private var modulePointer = UnsafeMutablePointer<OpaquePointer?>.allocate(capacity: 1)
nonisolated(unsafe) private var lookupPointer = UnsafeMutablePointer<OpaquePointer?>.allocate(capacity: 1)

@_cdecl("obs_module_set_pointer")
func obs_module_set_pointer(_ module: UnsafeMutableRawPointer) {
    modulePointer.pointee = OpaquePointer(module)
}

@_cdecl("obs_current_module")
func obs_current_module() -> OpaquePointer? {
    return modulePointer.pointee
}

@_cdecl("obs_module_ver")
func obs_module_ver() -> UInt32 {
    return libobsApiVersion
}

@_cdecl("obs_module_text")
func obs_module_text(_ val: UnsafePointer<CChar>) -> UnsafePointer<CChar>? {
    guard let lookup = lookupPointer.pointee else {
        return nil
    }

    let output = UnsafeMutablePointer<UnsafePointer<CChar>?>.allocate(capacity: 1)
    defer { output.deallocate() }
    text_lookup_getstr(lookup, val, output)

    return output.pointee
}

@_cdecl("obs_module_get_string")
func obs_module_get_string(val: UnsafePointer<CChar>, out: UnsafeMutablePointer<UnsafePointer<CChar>?>) -> Bool {
    guard let lookup = lookupPointer.pointee else {
        return false
    }

    return text_lookup_getstr(lookup, val, out)
}

@_cdecl("obs_module_set_locale")
func obs_module_set_locale(locale: UnsafePointer<CChar>) {
    if lookupPointer.pointee != nil {
        text_lookup_destroy(lookupPointer.pointee)
    }

    let lookup = obs_module_load_locale(obs_current_module(), "en-US", locale)

    lookupPointer.pointee = lookup
}

@_cdecl("obs_module_free_locale")
func obs_module_free_locale() {
    guard let lookup = lookupPointer.pointee else {
        return
    }

    text_lookup_destroy(lookup)
    lookupPointer.deallocate()
}

@_cdecl("obs_module_load")
func obs_module_load() -> Bool {
    OBSLog(.info, "Loaded")

    virtualcam_register_output()

    return true
}

@_cdecl("obs_module_unload")
func obs_module_unload() {
    OBSLog(.info, "Unloaded")
    modulePointer.deallocate()
}
