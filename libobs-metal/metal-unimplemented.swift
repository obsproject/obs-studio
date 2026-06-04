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

@_cdecl("device_load_default_samplerstate")
public func device_load_default_samplerstate(device: UnsafeRawPointer, b_3d: Bool, unit: Int) {
    return
}

@_cdecl("device_enter_context")
public func device_enter_context(device: UnsafeMutableRawPointer) {
    return
}

@_cdecl("device_leave_context")
public func device_leave_context(device: UnsafeMutableRawPointer) {
    return
}

@_cdecl("device_timer_create")
public func device_timer_create(device: UnsafeRawPointer) {
    return
}

@_cdecl("device_timer_range_create")
public func device_timer_range_create(device: UnsafeRawPointer) {
}

@_cdecl("gs_timer_destroy")
public func gs_timer_destroy(timer: UnsafeRawPointer) {
    return
}

@_cdecl("gs_timer_begin")
public func gs_timer_begin(timer: UnsafeRawPointer) {
    return
}

@_cdecl("gs_timer_end")
public func gs_timer_end(timer: UnsafeRawPointer) {
    return
}

@_cdecl("gs_timer_get_data")
public func gs_timer_get_data(timer: UnsafeRawPointer) -> Bool {
    return false
}

@_cdecl("gs_timer_range_destroy")
public func gs_timer_range_destroy(range: UnsafeRawPointer) {
    return
}

@_cdecl("gs_timer_range_begin")
public func gs_timer_range_begin(range: UnsafeRawPointer) {
    return
}

@_cdecl("gs_timer_range_end")
public func gs_timer_range_end(range: UnsafeRawPointer) {
    return
}

@_cdecl("gs_timer_range_get_data")
public func gs_timer_range_get_data(range: UnsafeRawPointer, disjoint: Bool, frequency: UInt64) -> Bool {
    return false
}

@_cdecl("device_debug_marker_begin")
public func device_debug_marker_begin(device: UnsafeRawPointer, monitor: UnsafeMutableRawPointer) {
    return
}

@_cdecl("device_debug_marker_end")
public func device_debug_marker_end(device: UnsafeRawPointer) {
    return
}

@_cdecl("device_set_cube_render_target")
public func device_set_cube_render_target(
    device: UnsafeRawPointer, cubetex: UnsafeRawPointer, side: Int, zstencil: UnsafeRawPointer
) {
    return
}
