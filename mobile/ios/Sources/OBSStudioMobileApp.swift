import SwiftUI

@main
struct OBSStudioMobileApp: App {
    var body: some Scene {
        WindowGroup {
            ObsMobileView()
                .preferredColorScheme(.dark)
        }
    }
}
