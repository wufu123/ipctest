import Foundation
import SwiftUI

/// 饮水数据存储，负责记录的增删与本地持久化
final class WaterStore: ObservableObject {
    @Published var records: [DrinkRecord] = [] {
        didSet { saveRecords() }
    }
    @Published var settings: ReminderSettings = ReminderSettings() {
        didSet { saveSettings() }
    }

    private let recordsKey = "waterRecords"
    private let settingsKey = "reminderSettings"

    init() {
        loadRecords()
        loadSettings()
    }

    // MARK: - 记录操作

    func addWater(amount: Int, type: DrinkType = .water) {
        records.append(DrinkRecord(amount: amount, type: type))
    }

    func delete(_ record: DrinkRecord) {
        records.removeAll { $0.id == record.id }
    }

    func records(for date: Date) -> [DrinkRecord] {
        records.filter { Calendar.current.isDate($0.date, inSameDayAs: date) }
    }

    func total(for date: Date) -> Int {
        records(for: date).reduce(0) { $0 + $1.amount }
    }

    var todayTotal: Int { total(for: .now) }

    var todayCount: Int { records(for: .now).count }

    /// 今日目标完成进度（0...1）
    var progress: Double {
        guard settings.dailyGoal > 0 else { return 0 }
        return min(Double(todayTotal) / Double(settings.dailyGoal), 1.0)
    }

    // MARK: - 持久化

    private func saveRecords() {
        if let data = try? JSONEncoder().encode(records) {
            UserDefaults.standard.set(data, forKey: recordsKey)
        }
    }

    private func loadRecords() {
        guard let data = UserDefaults.standard.data(forKey: recordsKey),
              let decoded = try? JSONDecoder().decode([DrinkRecord].self, from: data) else { return }
        records = decoded
    }

    private func saveSettings() {
        if let data = try? JSONEncoder().encode(settings) {
            UserDefaults.standard.set(data, forKey: settingsKey)
        }
    }

    private func loadSettings() {
        guard let data = UserDefaults.standard.data(forKey: settingsKey),
              let decoded = try? JSONDecoder().decode(ReminderSettings.self, from: data) else { return }
        settings = decoded
    }
}
