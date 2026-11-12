import SwiftUI

@main
struct WaterReminderApp: App {
    @StateObject private var store = WaterStore()
    @StateObject private var reminderManager = ReminderManager()

    var body: some Scene {
        WindowGroup {
            ContentView()
                .environmentObject(store)
                .environmentObject(reminderManager)
        }
    }
}
