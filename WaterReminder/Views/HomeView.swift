import SwiftUI

/// 今日饮水主界面：进度环 + 快捷添加
struct HomeView: View {
    @EnvironmentObject private var store: WaterStore

    var body: some View {
        NavigationStack {
            ScrollView {
                VStack(spacing: 32) {
                    ProgressRingView(
                        progress: store.progress,
                        total: store.todayTotal,
                        goal: store.settings.dailyGoal
                    )
                    .padding(.top, 16)

                    if store.todayCount > 0 {
                        Text("今日已记录 \(store.todayCount) 次")
                            .font(.footnote)
                            .foregroundStyle(.secondary)
                    }

                    quickAddSection
                }
                .padding(.horizontal)
                .padding(.bottom, 24)
            }
            .navigationTitle("今日饮水")
        }
    }

    private var quickAddSection: some View {
        VStack(alignment: .leading, spacing: 16) {
            Text("快速添加")
                .font(.headline)

            LazyVGrid(columns: [GridItem(.adaptive(minimum: 90), spacing: 12)], spacing: 12) {
                ForEach(CupSize.presets) { cup in
                    Button {
                        store.addWater(amount: cup.amount)
                    } label: {
                        VStack(spacing: 8) {
                            Image(systemName: "drop.fill")
                                .font(.title2)
                                .foregroundStyle(.blue)
                            Text(cup.name)
                                .font(.subheadline)
                                .fontWeight(.medium)
                            Text("\(cup.amount) ml")
                                .font(.caption)
                                .foregroundStyle(.secondary)
                        }
                        .frame(maxWidth: .infinity)
                        .padding(.vertical, 16)
                        .background(Color.blue.opacity(0.08), in: RoundedRectangle(cornerRadius: 16))
                    }
                    .buttonStyle(.plain)
                }
            }
        }
    }
}

/// 圆形进度环
struct ProgressRingView: View {
    let progress: Double
    let total: Int
    let goal: Int

    var body: some View {
        ZStack {
            Circle()
                .stroke(Color.blue.opacity(0.15), lineWidth: 20)

            Circle()
                .trim(from: 0, to: progress)
                .stroke(
                    Color.blue,
                    style: StrokeStyle(lineWidth: 20, lineCap: .round)
                )
                .rotationEffect(.degrees(-90))
                .animation(.spring(response: 0.5, dampingFraction: 0.8), value: progress)

            VStack(spacing: 4) {
                Text("\(total)")
                    .font(.system(size: 44, weight: .bold, design: .rounded))
                Text("/ \(goal) ml")
                    .font(.subheadline)
                    .foregroundStyle(.secondary)
            }
        }
        .frame(width: 220, height: 220)
    }
}
