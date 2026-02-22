#pragma once

#include "types.h"
#include "order_book.h"
#include <vector>

namespace hft {

// Matches incoming orders against the book.
// Limit orders rest if no cross; market orders walk the opposite side.
class MatchingEngine {
public:
    explicit MatchingEngine(OrderBook& book);

    std::vector<ExecutionReport> submitOrder(Order order, Timestamp current_ts);

    uint64_t totalOrdersProcessed() const { return total_orders_; }
    uint64_t totalFills() const { return total_fills_; }
    double totalVolume() const { return total_volume_; }

private:
    OrderBook& book_;
    uint64_t total_orders_ = 0;
    uint64_t total_fills_ = 0;
    double total_volume_ = 0.0;

    std::vector<ExecutionReport> matchBuy(Order& order, Timestamp ts);
    std::vector<ExecutionReport> matchSell(Order& order, Timestamp ts);
};

} // namespace hft
