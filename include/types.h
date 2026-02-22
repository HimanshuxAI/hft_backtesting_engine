#pragma once

#include <cstdint>
#include <string>
#include <chrono>
#include <cstring>

namespace hft {

enum class Side : uint8_t { BUY = 0, SELL = 1 };
enum class OrderType : uint8_t { LIMIT = 0, MARKET = 1 };

inline const char* sideToString(Side s) {
    return s == Side::BUY ? "BUY" : "SELL";
}

inline const char* orderTypeToString(OrderType t) {
    return t == OrderType::LIMIT ? "LIMIT" : "MARKET";
}

using Timestamp = uint64_t; // nanos since epoch
using Clock = std::chrono::high_resolution_clock;

inline Timestamp nowNs() {
    return static_cast<Timestamp>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(
            Clock::now().time_since_epoch()).count());
}

struct MarketUpdate {
    Timestamp timestamp = 0;
    char symbol[16] = {};
    double bid_price = 0.0;
    double ask_price = 0.0;
    uint32_t bid_qty = 0;
    uint32_t ask_qty = 0;
    double last_price = 0.0;
    uint32_t last_qty = 0;

    void setSymbol(const std::string& sym) {
        size_t len = std::min(sym.size(), sizeof(symbol) - 1);
        std::memcpy(symbol, sym.c_str(), len);
        symbol[len] = '\0';
    }
};

struct Order {
    uint64_t id = 0;
    char symbol[16] = {};
    Side side = Side::BUY;
    OrderType type = OrderType::LIMIT;
    double price = 0.0;
    uint32_t quantity = 0;
    Timestamp timestamp = 0;

    void setSymbol(const std::string& sym) {
        size_t len = std::min(sym.size(), sizeof(symbol) - 1);
        std::memcpy(symbol, sym.c_str(), len);
        symbol[len] = '\0';
    }
};

struct ExecutionReport {
    uint64_t order_id = 0;
    Side side = Side::BUY;
    double fill_price = 0.0;
    uint32_t fill_qty = 0;
    uint32_t remaining_qty = 0;
    Timestamp exchange_ts = 0;
    Timestamp receive_ts = 0;   // adjusted by latency model
    bool is_full_fill = false;

    double notional() const { return fill_price * fill_qty; }
};

struct PortfolioSnapshot {
    double cash = 0.0;
    int64_t position = 0;       // +long / -short
    double unrealized_pnl = 0.0;
    double realized_pnl = 0.0;
    double equity = 0.0;
    double mark_price = 0.0;
    Timestamp timestamp = 0;
};

// poison pill for end-of-stream signaling through queues
inline MarketUpdate makePoison() {
    MarketUpdate m;
    m.timestamp = UINT64_MAX;
    return m;
}

inline bool isPoison(const MarketUpdate& m) {
    return m.timestamp == UINT64_MAX;
}

} // namespace hft
