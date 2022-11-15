//
//  main.swift
//  mac-camera-extension
//
//  Created by Sebastian Beckmann on 30.09.22.
//  Changed by Patrick Heyer on 16.10.22.
//

import CoreMediaIO
import Foundation
import os.log

if #available(macOS 13.0, *) {
    let OBSCameraDeviceUUID =
        Bundle.main.object(forInfoDictionaryKey: "OBSCameraDeviceUUID") as? String
    let OBSCameraStreamUUID =
        Bundle.main.object(forInfoDictionaryKey: "OBSCameraStreamUUID") as? String
    let OBSCameraStreamSinkUUID =
        Bundle.main.object(forInfoDictionaryKey: "OBSCameraStreamSinkUUID") as? String

    guard let OBSCameraDeviceUUID = OBSCameraDeviceUUID,
        let OBSCameraStreamUUID = OBSCameraStreamUUID,
        let OBSCameraStreamSinkUUID = OBSCameraStreamSinkUUID
    else {
        fatalError("Unable to retrieve Camera Extension UUIDs from Info.plist.")
    }

    guard let deviceUUID = UUID(uuidString: OBSCameraDeviceUUID),
        let streamUUID = UUID(uuidString: OBSCameraStreamUUID),
        let streamSinkUUID = UUID(uuidString: OBSCameraStreamSinkUUID)
    else {
        fatalError("Unable to generate Camera Extension UUIDs from Info.plist values.")
    }

    let providerSource = OBSCameraProviderSource(
        clientQueue: nil,
        deviceUUID: deviceUUID,
        streamUUID: streamUUID,
        streamSinkUUID: streamSinkUUID
    )
    CMIOExtensionProvider.startService(provider: providerSource.provider)
    CFRunLoopRun()
} else {
    os_log("CMIO Camera Extensions require macOS 13.0 or newer.")
}
