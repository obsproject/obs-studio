//
//  OBSCameraDeviceSource.swift
//  camera-extension
//
//  Created by Sebastian Beckmann on 2022-09-30.
//  Changed by Patrick Heyer on 2022-10-16.
//

import AppKit
import CoreMediaIO
import Foundation
import IOKit.audio
import os.log

let OBSCameraFrameRate: Int = 60

class OBSCameraDeviceSource: NSObject, CMIOExtensionDeviceSource {
    private(set) var device: CMIOExtensionDevice!

    private var _streamSource: OBSCameraStreamSource!
    private var _streamSink: OBSCameraStreamSink!

    private var _streamingCounter: UInt32 = 0
    private var _streamingSinkCounter: UInt32 = 0

    private var _timer: DispatchSourceTimer?

    private let _timerQueue = DispatchQueue(
        label: "timerQueue",
        qos: .userInteractive,
        attributes: [],
        autoreleaseFrequency: .workItem,
        target: .global(qos: .userInteractive)
    )

    private var _videoDescription: CMFormatDescription!
    private var _bufferPool: CVPixelBufferPool!
    private var _bufferAuxAttributes: NSDictionary!

    private var _placeholderImage: NSImage!

    init(localizedName: String, deviceUUID: UUID, sourceUUID: UUID, sinkUUID: UUID) {
        super.init()

        self.device = CMIOExtensionDevice(
            localizedName: localizedName,
            deviceID: deviceUUID,
            legacyDeviceID: nil,
            source: self
        )

        let dimensions = CMVideoDimensions(width: 1920, height: 1080)
        CMVideoFormatDescriptionCreate(
            allocator: kCFAllocatorDefault,
            codecType: kCVPixelFormatType_32BGRA,
            width: dimensions.width,
            height: dimensions.height,
            extensions: nil,
            formatDescriptionOut: &_videoDescription
        )

        let pixelBufferAttributes: NSDictionary = [
            kCVPixelBufferWidthKey: dimensions.width,
            kCVPixelBufferHeightKey: dimensions.height,
            kCVPixelBufferPixelFormatTypeKey: _videoDescription.mediaSubType,
            kCVPixelBufferIOSurfacePropertiesKey: [CFString: CFTypeRef](),
        ]

        CVPixelBufferPoolCreate(kCFAllocatorDefault, nil, pixelBufferAttributes, &_bufferPool)

        let videoStreamFormat = CMIOExtensionStreamFormat.init(
            formatDescription: _videoDescription,
            maxFrameDuration: CMTime(value: 1, timescale: Int32(OBSCameraFrameRate)),
            minFrameDuration: CMTime(value: 1, timescale: Int32(OBSCameraFrameRate)),
            validFrameDurations: nil
        )
        _bufferAuxAttributes = [kCVPixelBufferPoolAllocationThresholdKey: 5]

        _streamSource = OBSCameraStreamSource(
            localizedName: "OBS Camera Extension Stream Source",
            streamID: sourceUUID,
            streamFormat: videoStreamFormat,
            device: device
        )

        _streamSink = OBSCameraStreamSink(
            localizedName: "OBS Camera Extension Stream Sink",
            streamID: sinkUUID,
            streamFormat: videoStreamFormat,
            device: device
        )

        do {
            try device.addStream(_streamSource.stream)
            try device.addStream(_streamSink.stream)
        } catch let error {
            fatalError("Failed to add stream: \(error.localizedDescription)")
        }

        let placeholderURL = Bundle.main.url(forResource: "placeholder", withExtension: "png")
        if let placeholderURL = placeholderURL {
            if let image = NSImage(contentsOf: placeholderURL) {
                _placeholderImage = image
            } else {
                fatalError("Unable to create NSImage from placeholder image in bundle resources")
            }
        } else {
            fatalError("Unable to find placeholder image in bundle resources")
        }
    }

    var availableProperties: Set<CMIOExtensionProperty> {
        return [.deviceTransportType, .deviceModel]
    }

    func deviceProperties(forProperties properties: Set<CMIOExtensionProperty>) throws
        -> CMIOExtensionDeviceProperties
    {
        let deviceProperties = CMIOExtensionDeviceProperties(dictionary: [:])
        if properties.contains(.deviceTransportType) {
            deviceProperties.transportType = kIOAudioDeviceTransportTypeVirtual
        }
        if properties.contains(.deviceModel) {
            deviceProperties.model = "OBS Camera Extension"
        }

        return deviceProperties
    }

    func setDeviceProperties(_ deviceProperties: CMIOExtensionDeviceProperties) throws {
    }

    func startStreaming() {
        guard let _ = _bufferPool else {
            return
        }

        _streamingCounter += 1

        _timer = DispatchSource.makeTimerSource(flags: .strict, queue: _timerQueue)

        _timer!.schedule(
            deadline: .now(),
            repeating: Double(1 / OBSCameraFrameRate),
            leeway: .seconds(0)
        )

        _timer!.setEventHandler {
            if self.sinkStarted {
                return
            }

            var error: CVReturn = noErr
            var pixelBuffer: CVPixelBuffer?
            error = CVPixelBufferPoolCreatePixelBufferWithAuxAttributes(
                kCFAllocatorDefault,
                self._bufferPool,
                self._bufferAuxAttributes,
                &pixelBuffer
            )

            if error == kCVReturnPoolAllocationFailed {
                os_log(.error, "no available PixelBuffers in PixelBufferPool: \(error)")
            }

            if let pixelBuffer = pixelBuffer {
                CVPixelBufferLockBaseAddress(pixelBuffer, [])

                let bufferPointer = CVPixelBufferGetBaseAddress(pixelBuffer)!

                let width = CVPixelBufferGetWidth(pixelBuffer)
                let height = CVPixelBufferGetHeight(pixelBuffer)
                let rowBytes = CVPixelBufferGetBytesPerRow(pixelBuffer)

                let cgContext = CGContext(
                    data: bufferPointer,
                    width: width,
                    height: height,
                    bitsPerComponent: 8,
                    bytesPerRow: rowBytes,
                    space: CGColorSpaceCreateDeviceRGB(),
                    bitmapInfo: CGImageAlphaInfo.noneSkipFirst.rawValue | CGBitmapInfo.byteOrder32Little.rawValue
                )!
                let graphicsContext = NSGraphicsContext(cgContext: cgContext, flipped: false)

                NSGraphicsContext.saveGraphicsState()
                NSGraphicsContext.current = graphicsContext
                self._placeholderImage.draw(in: CGRect(x: 0, y: 0, width: width, height: height))
                NSGraphicsContext.restoreGraphicsState()

                CVPixelBufferUnlockBaseAddress(pixelBuffer, [])

                var sampleBuffer: CMSampleBuffer!
                var timingInfo = CMSampleTimingInfo()

                timingInfo.presentationTimeStamp = CMClockGetTime(CMClockGetHostTimeClock())

                error = CMSampleBufferCreateForImageBuffer(
                    allocator: kCFAllocatorDefault,
                    imageBuffer: pixelBuffer,
                    dataReady: true,
                    makeDataReadyCallback: nil,
                    refcon: nil,
                    formatDescription: self._videoDescription,
                    sampleTiming: &timingInfo,
                    sampleBufferOut: &sampleBuffer
                )

                if error == noErr {
                    self._streamSource.stream.send(
                        sampleBuffer,
                        discontinuity: [],
                        hostTimeInNanoseconds: UInt64(
                            timingInfo.presentationTimeStamp.seconds * Double(NSEC_PER_SEC)
                        )
                    )
                }
            }
        }
        _timer!.setCancelHandler {}

        _timer!.resume()
    }

    func stopStreaming() {
        if _streamingCounter > 1 {
            _streamingCounter -= 1
        } else {
            _streamingCounter = 0

            if let timer = _timer {
                timer.cancel()
                _timer = nil
            }
        }
    }

    var sinkStarted = false
    var lastTimingInfo = CMSampleTimingInfo()

    func consumeBuffer(_ client: CMIOExtensionClient) {
        if !sinkStarted {
            return
        }

        self._streamSink.stream.consumeSampleBuffer(from: client) {
            sampleBuffer, sequenceNumber, discontinuity, hasMoreSampleBuffers, error in
            if sampleBuffer != nil {
                self.lastTimingInfo.presentationTimeStamp = CMClockGetTime(
                    CMClockGetHostTimeClock())
                let output: CMIOExtensionScheduledOutput = CMIOExtensionScheduledOutput(
                    sequenceNumber: sequenceNumber,
                    hostTimeInNanoseconds: UInt64(
                        self.lastTimingInfo.presentationTimeStamp.seconds * Double(NSEC_PER_SEC)
                    )
                )

                if self._streamingCounter > 0 {
                    self._streamSource.stream.send(
                        sampleBuffer!,
                        discontinuity: [],
                        hostTimeInNanoseconds: UInt64(
                            sampleBuffer!.presentationTimeStamp.seconds * Double(NSEC_PER_SEC)
                        )
                    )
                }

                self._streamSink.stream.notifyScheduledOutputChanged(output)
            }
            self.consumeBuffer(client)
        }
    }

    func startStreamingSink(client: CMIOExtensionClient) {
        _streamingSinkCounter += 1
        self.sinkStarted = true
        consumeBuffer(client)
    }

    func stopStreamingSink() {
        self.sinkStarted = false
        if _streamingCounter > 1 {
            _streamingSinkCounter -= 1
        } else {
            _streamingSinkCounter = 0
        }
    }
}
