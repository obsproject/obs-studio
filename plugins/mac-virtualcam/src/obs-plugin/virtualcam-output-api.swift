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

private func removeDalPlugin() throws {
    let fileManager = FileManager.default
    let dalPluginFileName = "/Library/CoreMediaIO/Plug-Ins/DAL/obs-mac-virtualcam.plugin"

    guard fileManager.fileExists(atPath: dalPluginFileName) else {
        return
    }

    let scriptSource = "do shell script \"rm -rf \(dalPluginFileName)\" with administrator privileges"
    guard let scriptObject = NSAppleScript(source: scriptSource) else {
        throw VirtualCamError.AppleScriptObject
    }

    var errorInfo: NSDictionary?

    scriptObject.executeAndReturnError(&errorInfo)

    if let errorInfo {
        let errorMessage = errorInfo.object(forKey: NSAppleScript.errorMessage) ?? ""
        OBSLog(.info, "The legacy DAL plugin is present on the system and could not be removed.\n\(errorMessage)")
        throw VirtualCamError.DALPluginRemoval
    }
}

@_cdecl("virtualcam_output_get_name")
func virtualcam_output_get_name(_ data: UnsafeMutableRawPointer?) -> UnsafePointer<CChar>? {
    let result = obs_module_text("Plugin.Name")

    return result
}

@_cdecl("virtualcam_output_create")
func virtualcam_output_create(_ settings: OpaquePointer?, _ output: OpaquePointer?) -> UnsafeMutableRawPointer? {
    guard let output else {
        return nil
    }

    let virtualCam = VirtualCam(output)

    virtualCam.activate()

    return Unmanaged.passRetained(virtualCam).toOpaque()
}

@_cdecl("virtualcam_output_destroy")
func virtualcam_output_destroy(_ data: UnsafeMutableRawPointer?) {
    guard let data else {
        return
    }

    let _ = retained(data) as VirtualCam
}

@_cdecl("virtualcam_output_start")
func virtualcam_output_start(_ data: UnsafeMutableRawPointer?) -> Bool {
    guard let data else {
        return false
    }

    let virtualCam: VirtualCam = unretained(data)

    do {
        try removeDalPlugin()
    } catch {
        obs_output_set_last_error(virtualCam.output, obs_module_text("Error.DAL.NotUninstalled"))
    }

    #if !DEBUG
        guard virtualCam.isInstalled else {
            if let lastErrorMessage = virtualCam.lastErrorMessage {
                let templatePointer = obs_module_text("Error.SystemExtension.InstallationError")

                guard let templatePointer else {
                    return false
                }

                let template = String(cString: templatePointer)
                var errorMessage = String()
                errorMessage.reserveCapacity(lastErrorMessage.count + template.count)
                errorMessage.append(template)
                errorMessage.append("\n\n")
                errorMessage.append(lastErrorMessage)

                obs_output_set_last_error(virtualCam.output, errorMessage)
            } else {
                obs_output_set_last_error(
                    virtualCam.output, obs_module_text("Error.SystemExtension.NotInstalled"))
            }

            return false
        }
    #else
        if !virtualCam.isInstalled {
            OBSLog(
                .warning,
                "Camera extension was not installed. OBS Studio will attempt to use an existing camera extension.")
            virtualCam.isInstalled = true
        }
    #endif

    var videoInfo = obs_video_info()
    obs_get_video_info(&videoInfo)

    var videoScaleInfo = video_scale_info(
        format: videoInfo.output_format,
        width: videoInfo.output_width,
        height: videoInfo.output_height,
        range: videoInfo.range,
        colorspace: videoInfo.colorspace)

    var videoFormat = videoInfo.coreVideoFormat

    if videoFormat != nil {
        obs_output_set_video_conversion(virtualCam.output, &videoScaleInfo)
    } else {
        OBSLog(
            .warning, "Selected output format (%s) not supported by CoreVideo, falling back to CPU conversion",
            get_video_format_name(videoInfo.output_format))

        videoScaleInfo.format = VIDEO_FORMAT_NV12
        videoInfo.output_format = VIDEO_FORMAT_NV12
        obs_output_set_video_conversion(virtualCam.output, &videoScaleInfo)
        videoFormat = videoInfo.coreVideoFormat
    }

    let config = VirtualCamOutputConfig(
        width: Int(videoInfo.output_width),
        height: Int(videoInfo.output_height),
        format: videoFormat!)

    do {
        try virtualCam.start(config)
    } catch VirtualCamError.PixelBufferPoolCreationFailed {
        OBSLog(.error, "Unable to create pixel buffer pool")
        return false
    } catch VirtualCamError.NoCameraDeviceUUID {
        OBSLog(.error, "Unable to get virtual camera device UUID from bundle")
        return false
    } catch VirtualCamError.CMIODeviceQueryError, VirtualCamError.NoMatchedCameraDevice {
        obs_output_set_last_error(virtualCam.output, obs_module_text("Error.SystemExtension.CameraUnavailable"))

        return false
    } catch VirtualCamError.CMIOStreamQueryError, VirtualCamError.DeviceStreamCount, VirtualCamError.CMIODeviceStart {
        obs_output_set_last_error(virtualCam.output, obs_module_text("Error.SystemExtension.CameraNotStarted"))

        return false
    } catch {
        OBSLog(.error, "Unknown camera extension startup failure")
        return false
    }

    if !obs_output_begin_data_capture(virtualCam.output, 0) {
        return false
    }

    return true
}

@_cdecl("virtualcam_output_stop")
func virtualcam_output_stop(_ data: UnsafeMutableRawPointer?, ts: UInt64) {
    guard let data else {
        return
    }

    let virtualCam: VirtualCam = unretained(data)

    obs_output_end_data_capture(virtualCam.output)

    virtualCam.stop()
}

@_cdecl("virtualcam_output_raw_video")
func virtualcam_output_raw_video(_ data: UnsafeMutableRawPointer?, frame: UnsafeMutablePointer<video_data>?) {
    guard let data, let frame else {
        return
    }

    let virtualCam: VirtualCam = unretained(data)

    do {
        try virtualCam.outputFrame(frame: frame)
    } catch VirtualCamError.NoPixelBufferPool {
        OBSLog(.error, "No pixel buffer pool available")
        return
    } catch VirtualCamError.PixelBufferCreation {
        OBSLog(.error, "Unable to create pixel buffer for video frame")
        return
    } catch VirtualCamError.SourceFrameData {
        OBSLog(.error, "No source frame data provided")
        return
    } catch {
        OBSLog(.error, "Unknown frame output error occurred")
        return
    }
}

func virtualcam_register_output() {
    let moduleId = "virtualcam_output"

    moduleId.withCString {
        var outputInfo = obs_output_info()

        outputInfo.id = $0
        outputInfo.flags = UInt32(OBS_OUTPUT_VIDEO)
        outputInfo.get_name = virtualcam_output_get_name
        outputInfo.create = virtualcam_output_create
        outputInfo.destroy = virtualcam_output_destroy
        outputInfo.start = virtualcam_output_start
        outputInfo.stop = virtualcam_output_stop
        outputInfo.raw_video = virtualcam_output_raw_video

        obs_register_output_s(&outputInfo, MemoryLayout<obs_output_info>.size)
    }
}
