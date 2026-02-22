#include "mean_reversion_strategy.h"
#include <numeric>
#include <iostream>
#include <iomanip>

namespace hft {

MeanReversionStrategy::MeanReversionStrategy() : MeanReversionStrategy(Config{}) {}

MeanReversionStrategy::MeanReversionStrategy(const Config& config)
    : config_(config) {}

void MeanReversionStrategy::onStart() {
    std::cout << "\n[Strategy] MeanReversion | window=" << config_.window_size
              << " k=" << std::fixed << std::setprecision(2) << config_.k_factor
              << " qty=" << config_.order_qty
              << " max_pos=" << config_.max_position << "\n\n";
}

void MeanReversionStrategy::onEnd() {
    std::cout << "\n[Strategy] done. trades=" << trades_
              << " ticks=" << ticks_
              << " final_pos=" << position_ << "\n";
}

std::vector<Order> MeanReversionStrategy::onMarketUpdate(const MarketUpdate& update) {
    std::vector<Order> orders;
    ++ticks_;

    double mid = (update.bid_price + update.ask_price) / 2.0;
    if (mid <= 0.0) return orders;

    price_window_.push_back(mid);
    if (price_window_.size() > config_.window_size)
        price_window_.pop_front();

    // wait for full window
    if (price_window_.size() < config_.window_size)
        return orders;

    double mean = rollingMean();
    double sd = rollingStdDev(mean);
    if (sd < 1e-9) return orders;

    double z = (mid - mean) / sd;

    if (z < -config_.k_factor && position_ < config_.max_position) {
        Order o;
        o.id = next_order_id_++;
        o.setSymbol(update.symbol);
        o.side = Side::BUY;
        o.type = OrderType::MARKET;
        o.price = update.ask_price;
        o.quantity = config_.order_qty;
        o.timestamp = update.timestamp;
        orders.push_back(o);
    } else if (z > config_.k_factor && position_ > -config_.max_position) {
        Order o;
        o.id = next_order_id_++;
        o.setSymbol(update.symbol);
        o.side = Side::SELL;
        o.type = OrderType::MARKET;
        o.price = update.bid_price;
        o.quantity = config_.order_qty;
        o.timestamp = update.timestamp;
        orders.push_back(o);
    }

    return orders;
}

void MeanReversionStrategy::onExecutionReport(const ExecutionReport& report) {
    ++trades_;
    if (report.side == Side::BUY)
        position_ += report.fill_qty;
    else
        position_ -= report.fill_qty;
}

double MeanReversionStrategy::rollingMean() const {
    return std::accumulate(price_window_.begin(), price_window_.end(), 0.0) /
           static_cast<double>(price_window_.size());
}

double MeanReversionStrategy::rollingStdDev(double mean) const {
    double sq = 0.0;
    for (double p : price_window_) {
        double d = p - mean;
        sq += d * d;
    }
    return std::sqrt(sq / static_cast<double>(price_window_.size()));
}

} // namespace hft
