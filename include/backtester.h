#pragma once

#include "types.h"
#include "order_book.h"
#include "matching_engine.h"
#include "latency_model.h"
#include "risk_metrics.h"
#include "strategy.h"
#include "csv_parser.h"
#include "spsc_queue.h"

#include <vector>
#include <memory>
#include <thread>
#include <atomic>

namespace hft {

// Tracks cash, position, PnL, equity curve through the backtest.
class PortfolioTracker {
public:
    explicit PortfolioTracker(double initial_cash = 1'000'000.0);

    void onFill(const ExecutionReport& report);
    void markToMarket(double current_price, Timestamp ts);

    PortfolioSnapshot snapshot() const { return current_; }
    const std::vector<double>& equityCurve() const { return equity_curve_; }
    const std::vector<double>& tradePnls() const { return trade_pnls_; }
    double initialCash() const { return initial_cash_; }

private:
    double initial_cash_;
    PortfolioSnapshot current_;
    double avg_entry_price_ = 0.0;

    std::vector<double> equity_curve_;
    std::vector<double> trade_pnls_;
};

// Main backtest driver. Runs a multi-threaded pipeline:
//   T1: data feed (csv -> spsc queue)
//   T2: strategy + matching + portfolio updates
class Backtester {
public:
    struct Config {
        std::string csv_path;
        double initial_cash = 1'000'000.0;
        LatencyModel::Config latency_config;
        bool verbose = false;
        size_t print_interval = 100;
    };

    Backtester(const Config& config, std::unique_ptr<IStrategy> strategy);
    RiskMetrics::Results run();

    const PortfolioTracker& portfolio() const { return portfolio_; }
    const OrderBook& orderBook() const { return book_; }

private:
    Config config_;
    std::unique_ptr<IStrategy> strategy_;

    OrderBook book_;
    MatchingEngine engine_;
    LatencyModel latency_;
    PortfolioTracker portfolio_;

    // inter-thread queues
    using DataQueue = SPSCQueue<MarketUpdate, 8192>;
    using OrderQueue = SPSCQueue<Order, 4096>;
    using FillQueue = SPSCQueue<ExecutionReport, 4096>;

    DataQueue data_queue_;
    OrderQueue order_queue_;
    FillQueue fill_queue_;

    std::atomic<bool> data_done_{false};
    std::atomic<bool> strategy_done_{false};
    std::atomic<uint64_t> ticks_processed_{0};

    void dataFeedWorker();
    void strategyWorker();

    RiskMetrics::Results runSingleThreaded();
    RiskMetrics::Results runMultiThreaded();
};

} // namespace hft
