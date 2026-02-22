#pragma once

#include "types.h"
#include <vector>

namespace hft {

class RiskMetrics {
public:
    struct Results {
        double sharpe_ratio = 0.0;
        double sortino_ratio = 0.0;
        double max_drawdown_pct = 0.0;
        size_t max_dd_duration = 0;
        double total_pnl = 0.0;
        double win_rate = 0.0;
        double profit_factor = 0.0;
        double avg_trade_pnl = 0.0;
        size_t total_trades = 0;
        size_t winning_trades = 0;
        size_t losing_trades = 0;
        double gross_profit = 0.0;
        double gross_loss = 0.0;
        double max_equity = 0.0;
        double min_equity = 0.0;
        double final_equity = 0.0;
    };

    // annualized sharpe/sortino (sqrt(252)), max drawdown, win rate, etc.
    static Results compute(
        const std::vector<double>& equity_curve,
        const std::vector<double>& trade_pnls,
        double risk_free_rate = 0.0);

    static void printReport(const Results& results);
};

} // namespace hft
