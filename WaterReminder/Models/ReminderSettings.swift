import Foundation

/// 提醒与目标设置
struct ReminderSettings: Codable, Equatable {
    /// 是否开启提醒
    var isEnabled: Bool = true
    /// 提醒间隔（分钟）
    var intervalMinutes: Int = 60
    /// 提醒开始时间（小时，0-23）
    var startHour: Int = 8
    /// 提醒结束时间（小时，0-23）
    var endHour: Int = 22
    /// 每日目标饮水量（毫升）
    var dailyGoal: Int = 2000
}
