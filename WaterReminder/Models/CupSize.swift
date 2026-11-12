import Foundation

/// 快捷杯子容量预设
struct CupSize: Identifiable, Hashable {
    let id: Int
    let name: String
    let amount: Int

    static let presets: [CupSize] = [
        CupSize(id: 0, name: "小杯", amount: 200),
        CupSize(id: 1, name: "中杯", amount: 300),
        CupSize(id: 2, name: "大杯", amount: 500),
        CupSize(id: 3, name: "一瓶", amount: 550)
    ]
}
