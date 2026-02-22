#pragma once

#include "types.h"
#include <map>
#include <deque>
#include <unordered_map>
#include <vector>
#include <functional>
#include <optional>

namespace hft {

using PriceLevel = std::deque<Order>;

// Price-time priority book. Bids descending, asks ascending.
class OrderBook {
public:
    OrderBook() = default;
    explicit OrderBook(const std::string& symbol);

    void addOrder(const Order& order);
    bool cancelOrder(uint64_t order_id);
    void updateFromMarket(const MarketUpdate& update);

    std::optional<double> getBestBid() const;
    std::optional<double> getBestAsk() const;
    double getSpread() const;
    double getMidPrice() const;

    struct DepthEntry {
        double price;
        uint32_t total_qty;
        size_t order_count;
    };
    std::vector<DepthEntry> getBidDepth(size_t levels = 5) const;
    std::vector<DepthEntry> getAskDepth(size_t levels = 5) const;

    size_t bidLevelCount() const { return bids_.size(); }
    size_t askLevelCount() const { return asks_.size(); }
    size_t totalOrderCount() const { return order_lookup_.size(); }

    const std::string& symbol() const { return symbol_; }
    void printBook(size_t depth = 5) const;

    using BidMap = std::map<double, PriceLevel, std::greater<double>>;
    using AskMap = std::map<double, PriceLevel, std::less<double>>;

    BidMap& bids() { return bids_; }
    AskMap& asks() { return asks_; }

private:
    std::string symbol_;
    BidMap bids_;
    AskMap asks_;

    struct OrderLocation {
        Side side;
        double price;
    };
    std::unordered_map<uint64_t, OrderLocation> order_lookup_;
};

} // namespace hft
