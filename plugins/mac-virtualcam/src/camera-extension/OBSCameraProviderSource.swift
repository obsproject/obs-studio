//
//  OBSCameraProviderSource.swift
//  mac-camera-extension
//
//  Created by Sebastian Beckmann on 30.09.22.
//  Changed by Patrick Heyer on 16.10.22.
//

import CoreMediaIO
import Foundation

@available(macOS 13.0, *)
class OBSCameraProviderSource: NSObject, CMIOExtensionProviderSource {
    private(set) var provider: CMIOExtensionProvider!

    private var deviceSource: OBSCameraDeviceSource!

    init(clientQueue: DispatchQueue?, deviceUUID: UUID, streamUUID: UUID, streamSinkUUID: UUID) {
        super.init()

        provider = CMIOExtensionProvider(source: self, clientQueue: clientQueue)
        deviceSource = OBSCameraDeviceSource(
            localizedName: "OBS Virtual Camera",
            deviceUUID: deviceUUID,
            streamUUID: streamUUID,
            streamSinkUUID: streamSinkUUID)
        do {
            try provider.addDevice(deviceSource.device)
        } catch let error {
            fatalError("Failed to add device \(error.localizedDescription)")
        }
    }

    func connect(to client: CMIOExtensionClient) throws {
    }

    func disconnect(from client: CMIOExtensionClient) {
    }

    var availableProperties: Set<CMIOExtensionProperty> {
        return [.providerName, .providerManufacturer]
    }

    func providerProperties(forProperties properties: Set<CMIOExtensionProperty>) throws
        -> CMIOExtensionProviderProperties
    {
        let providerProperties = CMIOExtensionProviderProperties(dictionary: [:])

        if properties.contains(.providerName) {
            providerProperties.name = "OBS Camera Extension Provider"
        }

        if properties.contains(.providerManufacturer) {
            providerProperties.manufacturer = "OBS Project"
        }

        return providerProperties
    }

    func setProviderProperties(_ providerProperties: CMIOExtensionProviderProperties) throws {
    }
}
