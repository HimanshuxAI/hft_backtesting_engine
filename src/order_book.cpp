#include "order_book.h"
#include <iostream>
#include <iomanip>
#include <algorithm>

namespace hft {

OrderBook::OrderBook(const std::string& symbol) : symbol_(symbol) {}

void OrderBook::addOrder(const Order& order) {
    if (order.type == OrderType::MARKET)
        return; // market orders go through matching engine, not into the book

    if (order.side == Side::BUY)
        bids_[order.price].push_back(order);
    else
        asks_[order.price].push_back(order);

    order_lookup_[order.id] = {order.side, order.price};
}

bool OrderBook::cancelOrder(uint64_t order_id) {
    auto it = order_lookup_.find(order_id);
    if (it == order_lookup_.end())
        return false;

    Side side = it->second.side;
    double price = it->second.price;

    auto removeFrom = [&](auto& side_map) {
        auto lvl = side_map.find(price);
        if (lvl != side_map.end()) {
            auto& q = lvl->second;
            q.erase(std::remove_if(q.begin(), q.end(),
                [order_id](const Order& o) { return o.id == order_id; }), q.end());
            if (q.empty()) side_map.erase(lvl);
        }
    };

    if (side == Side::BUY) removeFrom(bids_);
    else removeFrom(asks_);

    order_lookup_.erase(it);
    return true;
}

void OrderBook::updateFromMarket(const MarketUpdate& /*update*/) {
    // stub - book reconstruction from feed is handled in the backtester
    // by directly clearing and inserting synthetic BBO levels
}

std::optional<double> OrderBook::getBestBid() const {
    if (bids_.empty()) return std::nullopt;
    return bids_.begin()->first;
}

std::optional<double> OrderBook::getBestAsk() const {
    if (asks_.empty()) return std::nullopt;
    return asks_.begin()->first;
}

double OrderBook::getSpread() const {
    auto bid = getBestBid();
    auto ask = getBestAsk();
    if (!bid || !ask) return 0.0;
    return *ask - *bid;
}

double OrderBook::getMidPrice() const {
    auto bid = getBestBid();
    auto ask = getBestAsk();
    if (!bid || !ask) return 0.0;
    return (*bid + *ask) / 2.0;
}

std::vector<OrderBook::DepthEntry> OrderBook::getBidDepth(size_t levels) const {
    std::vector<DepthEntry> depth;
    depth.reserve(levels);
    size_t n = 0;
    for (const auto& [price, queue] : bids_) {
        if (n++ >= levels) break;
        uint32_t total = 0;
        for (const auto& o : queue) total += o.quantity;
        depth.push_back({price, total, queue.size()});
    }
    return depth;
}

std::vector<OrderBook::DepthEntry> OrderBook::getAskDepth(size_t levels) const {
    std::vector<DepthEntry> depth;
    depth.reserve(levels);
    size_t n = 0;
    for (const auto& [price, queue] : asks_) {
        if (n++ >= levels) break;
        uint32_t total = 0;
        for (const auto& o : queue) total += o.quantity;
        depth.push_back({price, total, queue.size()});
    }
    return depth;
}

void OrderBook::printBook(size_t depth) const {
    std::cout << "\n--- ORDER BOOK: " << symbol_ << " ---\n";

    auto asks = getAskDepth(depth);
    for (auto it = asks.rbegin(); it != asks.rend(); ++it) {
        std::cout << "  ASK  " << std::fixed << std::setprecision(2)
                  << std::setw(10) << it->price
                  << "  x " << std::setw(6) << it->total_qty << "\n";
    }

    std::cout << "  --- spread: " << std::fixed << std::setprecision(4)
              << getSpread() << " ---\n";

    auto bids = getBidDepth(depth);
    for (const auto& b : bids) {
        std::cout << "  BID  " << std::fixed << std::setprecision(2)
                  << std::setw(10) << b.price
                  << "  x " << std::setw(6) << b.total_qty << "\n";
    }
    std::cout << "\n";
}

} // namespace hft
