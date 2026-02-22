#pragma once

#include "strategy.h"
#include <deque>
#include <cmath>
#include <atomic>

namespace hft {

// Simple mean reversion: buy when z-score < -k, sell when > +k.
// Tracks rolling window of mid prices for mean/stddev.
class MeanReversionStrategy : public IStrategy {
public:
    struct Config {
        size_t window_size = 50;
        double k_factor = 1.5;
        uint32_t order_qty = 100;
        int64_t max_position = 500;  // risk limit
    };

    MeanReversionStrategy();
    explicit MeanReversionStrategy(const Config& config);

    std::vector<Order> onMarketUpdate(const MarketUpdate& update) override;
    void onExecutionReport(const ExecutionReport& report) override;
    void onStart() override;
    void onEnd() override;
    const char* name() const override { return "MeanReversion"; }

    int64_t position() const { return position_; }
    uint64_t tradesCount() const { return trades_; }

private:
    Config config_;
    std::deque<double> price_window_;

    int64_t position_ = 0;
    uint64_t next_order_id_ = 1;
    uint64_t trades_ = 0;
    uint64_t ticks_ = 0;

    double rollingMean() const;
    double rollingStdDev(double mean) const;
};

} // namespace hft
