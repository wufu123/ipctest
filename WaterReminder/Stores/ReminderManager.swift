import Foundation
import UserNotifications

/// 负责本地通知的授权与定时提醒调度
final class ReminderManager: ObservableObject {
    @Published var isAuthorized = false

    /// 请求通知权限
    func requestAuthorization(completion: ((Bool) -> Void)? = nil) {
        let center = UNUserNotificationCenter.current()
        center.requestAuthorization(options: [.alert, .sound, .badge]) { granted, _ in
            DispatchQueue.main.async {
                self.isAuthorized = granted
                completion?(granted)
            }
        }
    }

    /// 根据设置重新调度每日定时提醒
    func schedule(with settings: ReminderSettings) {
        let center = UNUserNotificationCenter.current()
        center.removeAllPendingNotificationRequests()

        guard settings.isEnabled, isAuthorized else { return }

        let interval = max(settings.intervalMinutes, 1)
        let startMinutes = settings.startHour * 60
        let endMinutes = settings.endHour * 60
        guard startMinutes <= endMinutes else { return }

        var minutes = startMinutes
        while minutes <= endMinutes {
            var components = DateComponents()
            components.hour = minutes / 60
            components.minute = minutes % 60

            let content = UNMutableNotificationContent()
            content.title = "该喝水啦"
            content.body = "记得补充水分，保持身体活力 💧"
            content.sound = .default

            let trigger = UNCalendarNotificationTrigger(dateMatching: components, repeats: true)
            let identifier = "water-reminder-\(components.hour!)-\(components.minute!)"
            let request = UNNotificationRequest(identifier: identifier, content: content, trigger: trigger)
            center.add(request)

            minutes += interval
        }
    }
}
