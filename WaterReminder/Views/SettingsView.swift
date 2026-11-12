import SwiftUI

/// 设置界面：每日目标 + 提醒配置
struct SettingsView: View {
    @EnvironmentObject private var store: WaterStore
    @EnvironmentObject private var reminderManager: ReminderManager

    var body: some View {
        NavigationStack {
            Form {
                Section("每日目标") {
                    Picker("目标饮水量", selection: $store.settings.dailyGoal) {
                        ForEach([1500, 2000, 2500, 3000, 3500], id: \.self) { value in
                            Text("\(value) ml").tag(value)
                        }
                    }
                }

                Section {
                    Toggle("开启提醒", isOn: $store.settings.isEnabled)

                    if store.settings.isEnabled {
                        Stepper(value: $store.settings.intervalMinutes, in: 15...180, step: 15) {
                            HStack {
                                Text("提醒间隔")
                                Spacer()
                                Text("\(store.settings.intervalMinutes) 分钟")
                                    .foregroundStyle(.secondary)
                            }
                        }

                        Stepper(value: $store.settings.startHour, in: 0...23) {
                            HStack {
                                Text("开始时间")
                                Spacer()
                                Text("\(store.settings.startHour):00")
                                    .foregroundStyle(.secondary)
                            }
                        }

                        Stepper(value: $store.settings.endHour, in: 0...23) {
                            HStack {
                                Text("结束时间")
                                Spacer()
                                Text("\(store.settings.endHour):00")
                                    .foregroundStyle(.secondary)
                            }
                        }
                    }
                } header: {
                    Text("提醒设置")
                } footer: {
                    if store.settings.isEnabled && !reminderManager.isAuthorized {
                        Text("未获得通知权限，请前往系统设置开启通知。")
                    }
                }
            }
            .navigationTitle("设置")
            .onAppear {
                updateReminders()
            }
            .onChange(of: store.settings) { _ in
                updateReminders()
            }
        }
    }

    private func updateReminders() {
        reminderManager.schedule(with: store.settings)
    }
}
