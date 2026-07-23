//
//  main.swift
//  camera-extension
//
//  Created by Sebastian Beckmann on 2022-09-30.
//  Changed by Patrick Heyer on 2022-10-16.
//

import CoreMediaIO
import Foundation
import os.log

let obsCameraDeviceUUID = Bundle.main.object(forInfoDictionaryKey: "OBSCameraDeviceUUID") as? String
let obsCameraSourceUUID = Bundle.main.object(forInfoDictionaryKey: "OBSCameraSourceUUID") as? String
let obsCameraSinkUUID = Bundle.main.object(forInfoDictionaryKey: "OBSCameraSinkUUID") as? String

guard let obsCameraDeviceUUID, let obsCameraSourceUUID, let obsCameraSinkUUID
else {
    fatalError("Unable to retrieve Camera Extension UUIDs from Info.plist.")
}

guard let deviceUUID = UUID(uuidString: obsCameraDeviceUUID), let sourceUUID = UUID(uuidString: obsCameraSourceUUID),
    let sinkUUID = UUID(uuidString: obsCameraSinkUUID)
else {
    fatalError("Unable to generate Camera Extension UUIDs from Info.plist values.")
}

let providerSource = OBSCameraProviderSource(
    clientQueue: nil, deviceUUID: deviceUUID, sourceUUID: sourceUUID, sinkUUID: sinkUUID)
CMIOExtensionProvider.startService(provider: providerSource.provider)
CFRunLoopRun()
