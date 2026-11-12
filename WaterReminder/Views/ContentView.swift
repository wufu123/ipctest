import SwiftUI

/// 主界面：Tab 切换今日 / 历史 / 设置
struct ContentView: View {
    @EnvironmentObject private var store: WaterStore
    @EnvironmentObject private var reminderManager: ReminderManager

    var body: some View {
        TabView {
            HomeView()
                .tabItem { Label("今日", systemImage: "drop.fill") }
            HistoryView()
                .tabItem { Label("历史", systemImage: "calendar") }
            SettingsView()
                .tabItem { Label("设置", systemImage: "gearshape.fill") }
        }
        .onAppear {
            reminderManager.requestAuthorization { _ in
                reminderManager.schedule(with: store.settings)
            }
        }
    }
}
