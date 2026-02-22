#pragma once

#include "types.h"
#include <vector>

namespace hft {

class IStrategy {
public:
    virtual ~IStrategy() = default;

    // called each tick; return orders to submit
    virtual std::vector<Order> onMarketUpdate(const MarketUpdate& update) = 0;
    virtual void onExecutionReport(const ExecutionReport& report) = 0;

    virtual void onStart() {}
    virtual void onEnd() {}
    virtual const char* name() const = 0;
};

} // namespace hft
