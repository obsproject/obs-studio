//
//  OBSCameraStreamSource.swift
//  mac-camera-extension
//
//  Created by Sebastian Beckmann on 30.09.22.
//  Changed by Patrick Heyer on 16.10.22.
//

import CoreMediaIO
import Foundation
import os.log

@available(macOS 13.0, *)
class OBSCameraStreamSource: NSObject, CMIOExtensionStreamSource {
    private(set) var stream: CMIOExtensionStream!

    let device: CMIOExtensionDevice

    private let _streamFormat: CMIOExtensionStreamFormat

    init(
        localizedName: String, streamID: UUID, streamFormat: CMIOExtensionStreamFormat,
        device: CMIOExtensionDevice
    ) {
        self.device = device
        self._streamFormat = streamFormat
        super.init()
        self.stream = CMIOExtensionStream(
            localizedName: localizedName,
            streamID: streamID,
            direction: .source,
            clockType: .hostTime,
            source: self
        )
    }

    var formats: [CMIOExtensionStreamFormat] {
        return [_streamFormat]
    }

    var activeFormatIndex: Int = 0 {
        didSet {
            if activeFormatIndex >= 1 {
                os_log(.error, "Invalid index")
            }
        }
    }

    var availableProperties: Set<CMIOExtensionProperty> {
        return [.streamActiveFormatIndex, .streamFrameDuration]
    }

    func streamProperties(forProperties properties: Set<CMIOExtensionProperty>) throws
        -> CMIOExtensionStreamProperties
    {
        let streamProperties = CMIOExtensionStreamProperties(dictionary: [:])

        if properties.contains(.streamActiveFormatIndex) {
            streamProperties.activeFormatIndex = 0
        }
        if properties.contains(.streamFrameDuration) {
            let frameDuration = CMTime(value: 1, timescale: Int32(OBSCameraFrameRate))
            streamProperties.frameDuration = frameDuration
        }

        return streamProperties
    }

    func setStreamProperties(_ streamProperties: CMIOExtensionStreamProperties) throws {
        if let activeFormatIndex = streamProperties.activeFormatIndex {
            self.activeFormatIndex = activeFormatIndex
        }
    }

    func authorizedToStartStream(for client: CMIOExtensionClient) -> Bool {
        return true
    }

    func startStream() throws {
        guard let deviceSource = device.source as? OBSCameraDeviceSource else {
            fatalError("Unexpected source type \(String(describing: device.source))")
        }

        deviceSource.startStreaming()
    }

    func stopStream() throws {
        guard let deviceSource = device.source as? OBSCameraDeviceSource else {
            fatalError("Unexpcted source type \(String(describing: device.source))")
        }
        deviceSource.stopStreaming()
    }
}
