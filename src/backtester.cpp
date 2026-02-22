#include "backtester.h"
#include <iostream>
#include <iomanip>
#include <chrono>

namespace hft {

// --- PortfolioTracker ---

PortfolioTracker::PortfolioTracker(double initial_cash)
    : initial_cash_(initial_cash)
{
    current_.cash = initial_cash;
    current_.equity = initial_cash;
    equity_curve_.reserve(8192);
    equity_curve_.push_back(initial_cash);
}

void PortfolioTracker::onFill(const ExecutionReport& report) {
    double cost = report.fill_price * report.fill_qty;

    if (report.side == Side::BUY) {
        current_.cash -= cost;
        int64_t old_pos = current_.position;
        current_.position += report.fill_qty;

        if (old_pos >= 0) {
            // adding to or opening a long
            double old_cost = avg_entry_price_ * old_pos;
            avg_entry_price_ = (old_cost + cost) / current_.position;
        } else if (current_.position <= 0) {
            // covering short
            double pnl = (avg_entry_price_ - report.fill_price) * report.fill_qty;
            current_.realized_pnl += pnl;
            trade_pnls_.push_back(pnl);
            if (current_.position == 0) avg_entry_price_ = 0.0;
        } else {
            // flipped short -> long
            double pnl = (avg_entry_price_ - report.fill_price) * (-old_pos);
            current_.realized_pnl += pnl;
            trade_pnls_.push_back(pnl);
            avg_entry_price_ = report.fill_price;
        }
    } else {
        current_.cash += cost;
        int64_t old_pos = current_.position;
        current_.position -= report.fill_qty;

        if (old_pos <= 0) {
            double old_cost = avg_entry_price_ * (-old_pos);
            avg_entry_price_ = (old_cost + cost) / (-current_.position);
        } else if (current_.position >= 0) {
            double pnl = (report.fill_price - avg_entry_price_) * report.fill_qty;
            current_.realized_pnl += pnl;
            trade_pnls_.push_back(pnl);
            if (current_.position == 0) avg_entry_price_ = 0.0;
        } else {
            // flipped long -> short
            double pnl = (report.fill_price - avg_entry_price_) * old_pos;
            current_.realized_pnl += pnl;
            trade_pnls_.push_back(pnl);
            avg_entry_price_ = report.fill_price;
        }
    }
}

void PortfolioTracker::markToMarket(double current_price, Timestamp ts) {
    current_.mark_price = current_price;
    current_.unrealized_pnl = 0.0;

    if (current_.position > 0)
        current_.unrealized_pnl = (current_price - avg_entry_price_) * current_.position;
    else if (current_.position < 0)
        current_.unrealized_pnl = (avg_entry_price_ - current_price) * (-current_.position);

    current_.equity = current_.cash + current_.unrealized_pnl
                      + current_.position * current_price;
    current_.timestamp = ts;
    equity_curve_.push_back(current_.equity);
}

// --- Backtester ---

Backtester::Backtester(const Config& config, std::unique_ptr<IStrategy> strategy)
    : config_(config),
      strategy_(std::move(strategy)),
      book_("BACKTEST"),
      engine_(book_),
      latency_(config.latency_config),
      portfolio_(config.initial_cash) {}

RiskMetrics::Results Backtester::run() {
    std::cout << "\n=== HFT Backtesting Engine ===\n"
              << "  data:     " << config_.csv_path << "\n"
              << "  capital:  $" << std::fixed << std::setprecision(0)
              << config_.initial_cash << "\n"
              << "  latency:  " << config_.latency_config.base_latency_ns / 1000
              << "us base + " << config_.latency_config.exchange_latency_ns / 1000
              << "us exchange\n"
              << "  strategy: " << strategy_->name() << "\n"
              << "==============================\n";

    auto t0 = std::chrono::high_resolution_clock::now();
    auto results = runMultiThreaded();
    auto t1 = std::chrono::high_resolution_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();

    std::cout << "\nBacktest done in " << ms << "ms"
              << " | " << ticks_processed_.load() << " ticks"
              << " | " << engine_.totalOrdersProcessed() << " orders"
              << " | " << engine_.totalFills() << " fills"
              << " | $" << std::fixed << std::setprecision(2)
              << engine_.totalVolume() << " volume\n";

    return results;
}

// builds synthetic liquidity from the current tick's BBO
static void refreshBookFromFeed(OrderBook& book, const MarketUpdate& update) {
    book.bids().clear();
    book.asks().clear();

    if (update.bid_price <= 0 || update.ask_price <= 0)
        return;

    Order bid;
    bid.id = 0;
    bid.side = Side::BUY;
    bid.type = OrderType::LIMIT;
    bid.price = update.bid_price;
    bid.quantity = update.bid_qty > 0 ? update.bid_qty : 1000;
    book.addOrder(bid);

    Order ask;
    ask.id = 0;
    ask.side = Side::SELL;
    ask.type = OrderType::LIMIT;
    ask.price = update.ask_price;
    ask.quantity = update.ask_qty > 0 ? update.ask_qty : 1000;
    book.addOrder(ask);
}

RiskMetrics::Results Backtester::runMultiThreaded() {
    strategy_->onStart();

    std::thread data_thread(&Backtester::dataFeedWorker, this);
    std::thread strat_thread(&Backtester::strategyWorker, this);

    // main thread just waits - strategy thread handles everything inline
    // (matching happens synchronously in the strategy thread for simplicity)
    data_thread.join();
    strat_thread.join();

    // drain leftover orders (shouldn't really happen but just in case)
    Order ord;
    while (order_queue_.try_pop(ord)) {
        auto reports = engine_.submitOrder(ord, ord.timestamp);
        for (auto& rpt : reports) {
            latency_.applyLatency(rpt);
            portfolio_.onFill(rpt);
            strategy_->onExecutionReport(rpt);
        }
    }

    strategy_->onEnd();

    return RiskMetrics::compute(portfolio_.equityCurve(), portfolio_.tradePnls());
}

void Backtester::dataFeedWorker() {
    CSVParser parser(config_.csv_path);
    parser.parse([this](const MarketUpdate& update) {
        data_queue_.push(update);
    });
    data_queue_.push(makePoison());
    data_done_.store(true, std::memory_order_release);
}

void Backtester::strategyWorker() {
    MarketUpdate update;
    uint64_t tick = 0;

    while (true) {
        if (!data_queue_.try_pop(update)) {
            if (data_done_.load(std::memory_order_acquire) && data_queue_.empty())
                break;
            continue;
        }

        if (isPoison(update)) break;

        ++tick;
        ticks_processed_.fetch_add(1, std::memory_order_relaxed);

        refreshBookFromFeed(book_, update);

        auto orders = strategy_->onMarketUpdate(update);

        for (auto& ord : orders) {
            auto reports = engine_.submitOrder(ord, update.timestamp);
            for (auto& rpt : reports) {
                latency_.applyLatency(rpt);
                portfolio_.onFill(rpt);
                strategy_->onExecutionReport(rpt);

                if (config_.verbose) {
                    std::cout << "[fill] " << sideToString(rpt.side)
                              << " " << rpt.fill_qty << " @ "
                              << std::fixed << std::setprecision(2) << rpt.fill_price
                              << " | lat=" << (rpt.receive_ts - rpt.exchange_ts) / 1000
                              << "us\n";
                }
            }
        }

        double mid = (update.bid_price + update.ask_price) / 2.0;
        portfolio_.markToMarket(mid, update.timestamp);

        if (config_.verbose && tick % config_.print_interval == 0) {
            book_.printBook(3);
            auto s = portfolio_.snapshot();
            std::cout << "[portfolio] cash=$" << std::fixed << std::setprecision(2)
                      << s.cash << " pos=" << s.position
                      << " equity=$" << s.equity
                      << " unreal=$" << s.unrealized_pnl
                      << " real=$" << s.realized_pnl << "\n";
        }
    }

    strategy_done_.store(true, std::memory_order_release);
}

RiskMetrics::Results Backtester::runSingleThreaded() {
    strategy_->onStart();

    CSVParser parser(config_.csv_path);
    auto updates = parser.parseAll();
    std::cout << "loaded " << updates.size() << " ticks\n";

    for (const auto& update : updates) {
        ticks_processed_.fetch_add(1, std::memory_order_relaxed);
        refreshBookFromFeed(book_, update);

        auto orders = strategy_->onMarketUpdate(update);
        for (auto& ord : orders) {
            auto reports = engine_.submitOrder(ord, update.timestamp);
            for (auto& rpt : reports) {
                latency_.applyLatency(rpt);
                portfolio_.onFill(rpt);
                strategy_->onExecutionReport(rpt);
            }
        }

        double mid = (update.bid_price + update.ask_price) / 2.0;
        portfolio_.markToMarket(mid, update.timestamp);
    }

    strategy_->onEnd();
    return RiskMetrics::compute(portfolio_.equityCurve(), portfolio_.tradePnls());
}

} // namespace hft
