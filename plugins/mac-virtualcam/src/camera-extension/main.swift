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

let OBSCameraDeviceUUID = Bundle.main.object(forInfoDictionaryKey: "OBSCameraDeviceUUID") as? String
let OBSCameraSourceUUID = Bundle.main.object(forInfoDictionaryKey: "OBSCameraSourceUUID") as? String
let OBSCameraSinkUUID = Bundle.main.object(forInfoDictionaryKey: "OBSCameraSinkUUID") as? String

guard let OBSCameraDeviceUUID = OBSCameraDeviceUUID, let OBSCameraSourceUUID = OBSCameraSourceUUID,
    let OBSCameraSinkUUID = OBSCameraSinkUUID
else {
    fatalError("Unable to retrieve Camera Extension UUIDs from Info.plist.")
}

guard let deviceUUID = UUID(uuidString: OBSCameraDeviceUUID), let sourceUUID = UUID(uuidString: OBSCameraSourceUUID),
    let sinkUUID = UUID(uuidString: OBSCameraSinkUUID)
else {
    fatalError("Unable to generate Camera Extension UUIDs from Info.plist values.")
}

let providerSource = OBSCameraProviderSource(
    clientQueue: nil, deviceUUID: deviceUUID, sourceUUID: sourceUUID, sinkUUID: sinkUUID)
CMIOExtensionProvider.startService(provider: providerSource.provider)
CFRunLoopRun()
