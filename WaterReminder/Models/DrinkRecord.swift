import Foundation

/// 饮品类型
enum DrinkType: String, Codable, CaseIterable, Identifiable {
    case water
    case tea
    case coffee
    case milk
    case juice

    var id: String { rawValue }

    var name: String {
        switch self {
        case .water: return "水"
        case .tea: return "茶"
        case .coffee: return "咖啡"
        case .milk: return "牛奶"
        case .juice: return "果汁"
        }
    }

    var emoji: String {
        switch self {
        case .water: return "💧"
        case .tea: return "🍵"
        case .coffee: return "☕️"
        case .milk: return "🥛"
        case .juice: return "🧃"
        }
    }
}

/// 单次饮水记录
struct DrinkRecord: Identifiable, Codable, Hashable {
    let id: UUID
    let amount: Int
    let date: Date
    let type: DrinkType

    init(id: UUID = UUID(), amount: Int, date: Date = .now, type: DrinkType = .water) {
        self.id = id
        self.amount = amount
        self.date = date
        self.type = type
    }
}
