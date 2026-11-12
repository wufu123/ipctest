import SwiftUI

/// 历史饮水记录，按日期分组展示
struct HistoryView: View {
    @EnvironmentObject private var store: WaterStore

    private var groupedRecords: [DailyGroup] {
        let grouped = Dictionary(grouping: store.records) { record in
            Calendar.current.startOfDay(for: record.date)
        }
        return grouped
            .map { DailyGroup(date: $0.key, records: $0.value.sorted { $0.date > $1.date }) }
            .sorted { $0.date > $1.date }
    }

    var body: some View {
        NavigationStack {
            List {
                ForEach(groupedRecords) { group in
                    Section {
                        ForEach(group.records) { record in
                            HStack {
                                Text(record.type.emoji)
                                    .font(.title3)
                                VStack(alignment: .leading, spacing: 2) {
                                    Text(record.type.name)
                                        .font(.body)
                                    Text(record.date, style: .time)
                                        .font(.caption)
                                        .foregroundStyle(.secondary)
                                }
                                Spacer()
                                Text("\(record.amount) ml")
                                    .font(.subheadline)
                                    .foregroundStyle(.secondary)
                            }
                        }
                        .onDelete { offsets in
                            delete(from: group.records, at: offsets)
                        }
                    } header: {
                        Text(group.date, format: .dateTime.month().day().weekday())
                    }
                }
            }
            .navigationTitle("历史记录")
            .overlay {
                if store.records.isEmpty {
                    VStack(spacing: 8) {
                        Image(systemName: "drop")
                            .font(.largeTitle)
                            .foregroundStyle(.secondary)
                        Text("暂无记录")
                            .foregroundStyle(.secondary)
                    }
                }
            }
        }
    }

    private func delete(from records: [DrinkRecord], at offsets: IndexSet) {
        for index in offsets {
            store.delete(records[index])
        }
    }
}

/// 按日期分组的历史记录
private struct DailyGroup: Identifiable {
    let date: Date
    let records: [DrinkRecord]
    var id: Date { date }
}
