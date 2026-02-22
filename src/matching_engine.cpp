#include "matching_engine.h"

namespace hft {

MatchingEngine::MatchingEngine(OrderBook& book) : book_(book) {}

std::vector<ExecutionReport> MatchingEngine::submitOrder(Order order, Timestamp current_ts) {
    ++total_orders_;
    order.timestamp = current_ts;

    auto reports = (order.side == Side::BUY)
        ? matchBuy(order, current_ts)
        : matchSell(order, current_ts);

    // leftover limit qty rests in the book
    if (order.type == OrderType::LIMIT && order.quantity > 0)
        book_.addOrder(order);

    return reports;
}

std::vector<ExecutionReport> MatchingEngine::matchBuy(Order& order, Timestamp ts) {
    std::vector<ExecutionReport> reports;
    auto& asks = book_.asks();

    while (order.quantity > 0 && !asks.empty()) {
        auto best = asks.begin();
        double px = best->first;

        if (order.type == OrderType::LIMIT && px > order.price)
            break;

        auto& queue = best->second;
        while (order.quantity > 0 && !queue.empty()) {
            auto& resting = queue.front();
            uint32_t fill_qty = std::min(order.quantity, resting.quantity);

            ExecutionReport rpt;
            rpt.order_id = order.id;
            rpt.side = Side::BUY;
            rpt.fill_price = px;
            rpt.fill_qty = fill_qty;
            rpt.exchange_ts = ts;
            rpt.receive_ts = ts; // latency model adjusts later

            order.quantity -= fill_qty;
            resting.quantity -= fill_qty;

            rpt.remaining_qty = order.quantity;
            rpt.is_full_fill = (order.quantity == 0);

            total_volume_ += rpt.notional();
            ++total_fills_;
            reports.push_back(rpt);

            if (resting.quantity == 0)
                queue.pop_front();
        }

        if (queue.empty())
            asks.erase(best);
    }
    return reports;
}

std::vector<ExecutionReport> MatchingEngine::matchSell(Order& order, Timestamp ts) {
    std::vector<ExecutionReport> reports;
    auto& bids = book_.bids();

    while (order.quantity > 0 && !bids.empty()) {
        auto best = bids.begin();
        double px = best->first;

        if (order.type == OrderType::LIMIT && px < order.price)
            break;

        auto& queue = best->second;
        while (order.quantity > 0 && !queue.empty()) {
            auto& resting = queue.front();
            uint32_t fill_qty = std::min(order.quantity, resting.quantity);

            ExecutionReport rpt;
            rpt.order_id = order.id;
            rpt.side = Side::SELL;
            rpt.fill_price = px;
            rpt.fill_qty = fill_qty;
            rpt.exchange_ts = ts;
            rpt.receive_ts = ts;

            order.quantity -= fill_qty;
            resting.quantity -= fill_qty;

            rpt.remaining_qty = order.quantity;
            rpt.is_full_fill = (order.quantity == 0);

            total_volume_ += rpt.notional();
            ++total_fills_;
            reports.push_back(rpt);

            if (resting.quantity == 0)
                queue.pop_front();
        }

        if (queue.empty())
            bids.erase(best);
    }
    return reports;
}

} // namespace hft
