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
import CoreVideo
import Foundation
import SystemExtensions

private let virtualCameraIdentifier = "com.obsproject.mac-virtualcam"
private let bundleKeyForUUID = "OBSCameraDeviceUUID"
private let cameraExtensionIdentifier = "com.obsproject.obs-studio.mac-camera-extension"

struct VirtualCamOutputConfig {
    let width: Int
    let height: Int
    let format: OSType
}

enum VirtualCamError: Error {
    case AppleScriptObject
    case CMIODeviceQueryError
    case CMIODeviceStart
    case CMIOStreamQueryError
    case DALPluginRemoval
    case DeviceStreamCount
    case NoCameraDeviceUUID
    case NoMatchedCameraDevice
    case NoPixelBufferPool
    case PixelBufferCreation
    case PixelBufferPoolCreationFailed
    case SourceFrameData
}

class VirtualCam: NSObject {
    var isInstalled = false
    var lastErrorMessage: String?
    let output: OpaquePointer
    var pixelBufferPool: CVPixelBufferPool?
    var deviceUuid: String?

    var queue: CMSimpleQueue?
    var queueReady: Bool = false
    var deviceId: CMIODeviceID?
    var streamId: CMIOStreamID?
    var format: CMFormatDescription?

    init(_ output: OpaquePointer) {
        self.output = output
    }

    func start(_ config: VirtualCamOutputConfig) throws {
        let status = CVPixelBufferPoolCreate(
            kCFAllocatorDefault,
            nil,
            [
                kCVPixelBufferPixelFormatTypeKey: config.format,
                kCVPixelBufferWidthKey: config.width,
                kCVPixelBufferHeightKey: config.height,
                kCVPixelBufferIOSurfacePropertiesKey: [:],
            ] as CFDictionary,
            &pixelBufferPool
        )

        guard status == kCVReturnSuccess else {
            throw VirtualCamError.PixelBufferPoolCreationFailed
        }

        guard
            let virtualCamUUID = Bundle(identifier: virtualCameraIdentifier)?.object(
                forInfoDictionaryKey: bundleKeyForUUID) as? String
        else {
            throw VirtualCamError.NoCameraDeviceUUID
        }

        let devicesProperty = CMIOObjectProperty(selector: kCMIOHardwarePropertyDevices, type: .array(.objectId))

        guard case .objectIds(let devices) = devicesProperty.get(scope: .global, element: .main, in: .system) else {
            throw VirtualCamError.CMIODeviceQueryError
        }

        for deviceObjectId in devices {
            let deviceUidProperty = CMIOObjectProperty(selector: kCMIODevicePropertyDeviceUID, type: .string)

            if case .string(let deviceUid) = deviceUidProperty.get(scope: .global, element: .main, in: deviceObjectId),
                deviceUid == virtualCamUUID
            {
                deviceUuid = deviceUid
                deviceId = deviceObjectId
                break
            }
        }

        guard deviceId != nil else {
            throw VirtualCamError.NoMatchedCameraDevice
        }

        let streamsProperty = CMIOObjectProperty(selector: kCMIODevicePropertyStreams, type: .array(.streamId))

        guard case .streamIds(let streams) = streamsProperty.get(scope: .global, element: .main, in: deviceId!) else {
            throw VirtualCamError.CMIOStreamQueryError
        }

        guard streams.count >= 2 else {
            throw VirtualCamError.DeviceStreamCount
        }

        streamId = streams[1]

        let bufferQueue = UnsafeMutablePointer<Unmanaged<CMSimpleQueue>?>.allocate(capacity: 1)
        let opaqueSelf = UnsafeMutableRawPointer(Unmanaged.passUnretained(self).toOpaque())
        var result = CMIOStreamCopyBufferQueue(
            streamId!,
            { (_: CMIOStreamID, _: UnsafeMutableRawPointer?, refcon: UnsafeMutableRawPointer?) in
                guard let refcon else {
                    return
                }

                let virtualCam = Unmanaged<VirtualCam>.fromOpaque(refcon).takeUnretainedValue()

                virtualCam.queueReady = true
            },
            opaqueSelf,
            bufferQueue
        )

        guard result == kCMIOHardwareNoError else {
            throw VirtualCamError.CMIODeviceStart
        }

        if let queue = bufferQueue.pointee {
            self.queue = queue.takeUnretainedValue()
        }

        CMVideoFormatDescriptionCreate(
            allocator: kCFAllocatorDefault,
            codecType: config.format,
            width: Int32(config.width),
            height: Int32(config.height),
            extensions: nil,
            formatDescriptionOut: &format
        )

        result = CMIODeviceStartStream(deviceId!, streamId!)

        guard result == kCMIOHardwareNoError else {
            throw VirtualCamError.CMIODeviceStart
        }
    }

    func stop() {
        guard let deviceId, let streamId else {
            return
        }
        CMIODeviceStopStream(deviceId, streamId)

        self.deviceId = 0
        self.streamId = 0
        pixelBufferPool = nil
    }

    func outputFrame(frame: UnsafeMutablePointer<video_data>) throws {
        guard pixelBufferPool != nil else {
            throw VirtualCamError.NoPixelBufferPool
        }

        var pixelBuffer: CVPixelBuffer?

        let status = CVPixelBufferPoolCreatePixelBuffer(
            kCFAllocatorDefault,
            pixelBufferPool!,
            &pixelBuffer
        )

        guard status == kCVReturnSuccess else {
            throw VirtualCamError.PixelBufferCreation
        }

        let numPlanes = CVPixelBufferGetPlaneCount(pixelBuffer!)
        CVPixelBufferLockBaseAddress(pixelBuffer!, .init(rawValue: 0))

        let sourceDataBuffer = withUnsafeMutablePointer(to: &frame.pointee.data) {
            $0.withMemoryRebound(to: UnsafeMutablePointer<UInt8>?.self, capacity: 8) {
                $0
            }
        }
        let sourceHeightBuffer = withUnsafeMutablePointer(to: &frame.pointee.linesize) {
            $0.withMemoryRebound(to: UInt32.self, capacity: 8) {
                $0
            }
        }

        for plane in 0..<numPlanes {
            guard let source = sourceDataBuffer[plane] else {
                throw VirtualCamError.SourceFrameData
            }

            guard let destination = CVPixelBufferGetBaseAddressOfPlane(pixelBuffer!, plane) else {
                throw VirtualCamError.PixelBufferCreation
            }

            let bytesPerRow = CVPixelBufferGetBytesPerRowOfPlane(pixelBuffer!, plane)
            let sourceBytesPerRow = sourceHeightBuffer[plane]
            let height = CVPixelBufferGetHeightOfPlane(pixelBuffer!, plane)

            if bytesPerRow == sourceBytesPerRow {
                destination.copyMemory(from: source, byteCount: bytesPerRow * height)
            } else {
                for line in 0..<height {
                    let sourceLine = source.advanced(by: line)
                    let destinationLine = destination.advanced(by: line)
                    destinationLine.copyMemory(from: sourceLine, byteCount: bytesPerRow)
                }
            }
        }

        CVPixelBufferUnlockBaseAddress(pixelBuffer!, .init(rawValue: 0))

        var sampleBuffer: CMSampleBuffer!
        var timingInfo = CMSampleTimingInfo(
            duration: .init(),
            presentationTimeStamp: .init(
                value: CMTimeValue(frame.pointee.timestamp), timescale: CMTimeScale(1_000_000_000)),
            decodeTimeStamp: .init()
        )

        CMSampleBufferCreateForImageBuffer(
            allocator: kCFAllocatorDefault,
            imageBuffer: pixelBuffer!,
            dataReady: true,
            makeDataReadyCallback: nil,
            refcon: nil,
            formatDescription: format!,
            sampleTiming: &timingInfo,
            sampleBufferOut: &sampleBuffer
        )

        if let sampleBuffer, let queue = self.queue {
            let bufferPointer = UnsafeMutableRawPointer(Unmanaged.passRetained(sampleBuffer).toOpaque())
            CMSimpleQueueEnqueue(queue, element: bufferPointer)
        }
    }

    func getRetained() -> OpaquePointer {
        let retained = Unmanaged.passRetained(self).toOpaque()

        return OpaquePointer(retained)
    }

    func getUnretained() -> OpaquePointer {
        let unretained = Unmanaged.passUnretained(self).toOpaque()

        return OpaquePointer(unretained)
    }
}

extension VirtualCam: OSSystemExtensionRequestDelegate {
    func activate() {
        let request = OSSystemExtensionRequest.activationRequest(
            forExtensionWithIdentifier: cameraExtensionIdentifier,
            queue: .main
        )

        request.delegate = self

        OSSystemExtensionManager.shared.submitRequest(request)
    }

    func request(
        _ request: OSSystemExtensionRequest, actionForReplacingExtension existing: OSSystemExtensionProperties,
        withExtension ext: OSSystemExtensionProperties
    ) -> OSSystemExtensionRequest.ReplacementAction {
        let logInfo = """
            Virtual Camera Extension replacement requested:
                - Existing version: \(existing.bundleShortVersion) (\(existing.bundleVersion))
                - New version: \(ext.bundleShortVersion) (\(ext.bundleVersion))
            """

        OBSLog(.info, logInfo)

        return .replace
    }

    func requestNeedsUserApproval(_ request: OSSystemExtensionRequest) {
        isInstalled = false
        OBSLog(.info, "Camera Extension user approval required.")
    }

    func request(_ request: OSSystemExtensionRequest, didFinishWithResult result: OSSystemExtensionRequest.Result) {
        switch result {
        case .completed:
            isInstalled = true

            OBSLog(.info, "Camera Extension activated successfully.")
        case .willCompleteAfterReboot:
            if let message = obs_module_text("Error.SystemExtension.CompleteAfterReboot") {
                lastErrorMessage = String(cString: message)
            }

            OBSLog(.info, "Camera Extension activation will complete after a reboot.")
        @unknown default:
            OBSLog(.warning, "Unknown Camera Extension activation result encountered.")
        }
    }

    func request(_ request: OSSystemExtensionRequest, didFailWithError error: any Error) {
        let severity: OBSLogLevel
        switch error {
        case OSSystemExtensionError.unsupportedParentBundleLocation:
            if let message = obs_module_text("Error.SystemExtension.WrongLocation") {
                lastErrorMessage = String(cString: message)
            }

            severity = .warning
        default:
            lastErrorMessage =
                "Unknown camera extension activation error '\((error as NSError).code)' (\"\(error.localizedDescription)\")"
            severity = .error
        }

        OBSLog(severity, lastErrorMessage!)
    }
}
