#include "risk_metrics.h"
#include <cmath>
#include <algorithm>
#include <numeric>
#include <iostream>
#include <iomanip>
#include <sstream>

namespace hft {

RiskMetrics::Results RiskMetrics::compute(
    const std::vector<double>& equity_curve,
    const std::vector<double>& trade_pnls,
    double risk_free_rate)
{
    Results r;
    r.total_trades = trade_pnls.size();

    if (r.total_trades == 0) {
        if (!equity_curve.empty()) {
            r.final_equity = equity_curve.back();
            r.max_equity = *std::max_element(equity_curve.begin(), equity_curve.end());
            r.min_equity = *std::min_element(equity_curve.begin(), equity_curve.end());
        }
        return r;
    }

    // trade-level stats
    for (double pnl : trade_pnls) {
        r.total_pnl += pnl;
        if (pnl > 0.0) { r.winning_trades++; r.gross_profit += pnl; }
        else if (pnl < 0.0) { r.losing_trades++; r.gross_loss += std::abs(pnl); }
    }

    r.win_rate = static_cast<double>(r.winning_trades) / r.total_trades;
    r.avg_trade_pnl = r.total_pnl / r.total_trades;
    r.profit_factor = (r.gross_loss > 1e-9) ? (r.gross_profit / r.gross_loss) : 999.99;

    if (equity_curve.size() < 2) return r;

    r.final_equity = equity_curve.back();
    r.max_equity = *std::max_element(equity_curve.begin(), equity_curve.end());
    r.min_equity = *std::min_element(equity_curve.begin(), equity_curve.end());

    // compute simple returns
    std::vector<double> returns;
    returns.reserve(equity_curve.size() - 1);
    for (size_t i = 1; i < equity_curve.size(); ++i) {
        if (equity_curve[i-1] > 1e-9)
            returns.push_back((equity_curve[i] - equity_curve[i-1]) / equity_curve[i-1]);
        else
            returns.push_back(0.0);
    }

    double mean_ret = std::accumulate(returns.begin(), returns.end(), 0.0) / returns.size();

    double sum_sq = 0.0;
    for (double ret : returns) {
        double d = ret - mean_ret;
        sum_sq += d * d;
    }
    double std_dev = std::sqrt(sum_sq / returns.size());

    double daily_rf = risk_free_rate / 252.0;

    // sharpe (annualized)
    if (std_dev > 1e-12)
        r.sharpe_ratio = ((mean_ret - daily_rf) / std_dev) * std::sqrt(252.0);

    // sortino (downside only)
    double downside_sq = 0.0;
    size_t down_n = 0;
    for (double ret : returns) {
        if (ret < daily_rf) {
            double d = ret - daily_rf;
            downside_sq += d * d;
            ++down_n;
        }
    }
    if (down_n > 0) {
        double downside_dev = std::sqrt(downside_sq / returns.size());
        if (downside_dev > 1e-12)
            r.sortino_ratio = ((mean_ret - daily_rf) / downside_dev) * std::sqrt(252.0);
    }

    // max drawdown
    double peak = equity_curve[0];
    double max_dd = 0.0;
    size_t cur_dd_len = 0, max_dd_len = 0;

    for (size_t i = 1; i < equity_curve.size(); ++i) {
        if (equity_curve[i] > peak) {
            peak = equity_curve[i];
            cur_dd_len = 0;
        } else {
            double dd = (peak - equity_curve[i]) / peak;
            ++cur_dd_len;
            if (dd > max_dd) {
                max_dd = dd;
                max_dd_len = cur_dd_len;
            }
        }
    }
    r.max_drawdown_pct = max_dd * 100.0;
    r.max_dd_duration = max_dd_len;

    return r;
}

void RiskMetrics::printReport(const Results& r) {
    std::cout << std::fixed << std::setprecision(4);

    std::cout << "\n========== RISK METRICS ==========\n";

    std::cout << "  Total PnL:        $" << r.total_pnl << "\n";
    std::cout << "  Final Equity:     $" << r.final_equity << "\n";
    std::cout << "  Avg Trade PnL:    $" << r.avg_trade_pnl << "\n";

    std::cout << "\n  Sharpe:           " << r.sharpe_ratio << "\n";
    std::cout << "  Sortino:          " << r.sortino_ratio << "\n";
    std::cout << "  Max Drawdown:     " << std::setprecision(2) << r.max_drawdown_pct << "%\n";
    std::cout << "  Max DD Duration:  " << r.max_dd_duration << " ticks\n";

    std::cout << std::setprecision(4);
    std::cout << "\n  Total Trades:     " << r.total_trades << "\n";
    std::cout << "  Winners:          " << r.winning_trades << "\n";
    std::cout << "  Losers:           " << r.losing_trades << "\n";
    std::cout << "  Win Rate:         " << std::setprecision(2) << (r.win_rate * 100.0) << "%\n";
    std::cout << "  Profit Factor:    " << std::setprecision(4) << r.profit_factor << "\n";
    std::cout << "  Gross Profit:     $" << r.gross_profit << "\n";
    std::cout << "  Gross Loss:       $" << r.gross_loss << "\n";

    std::cout << "\n  Peak Equity:      $" << r.max_equity << "\n";
    std::cout << "  Trough Equity:    $" << r.min_equity << "\n";
    std::cout << "==================================\n\n";
}

} // namespace hft
