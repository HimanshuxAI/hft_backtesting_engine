#include "backtester.h"
#include "mean_reversion_strategy.h"
#include "risk_metrics.h"

#include <iostream>
#include <string>
#include <cstring>
#include <memory>

static void usage(const char* prog) {
    std::cerr << "Usage: " << prog << " <csv_file> [options]\n\n"
              << "Options:\n"
              << "  --capital <amount>      Initial capital (default: 1000000)\n"
              << "  --window <size>         Rolling window ticks (default: 50)\n"
              << "  --k-factor <k>          Z-score threshold (default: 1.5)\n"
              << "  --order-qty <qty>       Shares per trade (default: 100)\n"
              << "  --max-position <pos>    Max net position (default: 500)\n"
              << "  --base-latency <ns>     Base latency in ns (default: 50000)\n"
              << "  --verbose               Print book snapshots and fills\n"
              << "  --help                  Show this message\n";
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        usage(argv[0]);
        return 1;
    }

    std::string csv_path = argv[1];
    if (csv_path == "--help" || csv_path == "-h") {
        usage(argv[0]);
        return 0;
    }

    hft::Backtester::Config bt_cfg;
    bt_cfg.csv_path = csv_path;

    hft::MeanReversionStrategy::Config strat_cfg;

    for (int i = 2; i < argc; ++i) {
        if (std::strcmp(argv[i], "--capital") == 0 && i+1 < argc) {
            bt_cfg.initial_cash = std::stod(argv[++i]);
        } else if (std::strcmp(argv[i], "--window") == 0 && i+1 < argc) {
            strat_cfg.window_size = std::stoull(argv[++i]);
        } else if (std::strcmp(argv[i], "--k-factor") == 0 && i+1 < argc) {
            strat_cfg.k_factor = std::stod(argv[++i]);
        } else if (std::strcmp(argv[i], "--order-qty") == 0 && i+1 < argc) {
            strat_cfg.order_qty = std::stoul(argv[++i]);
        } else if (std::strcmp(argv[i], "--max-position") == 0 && i+1 < argc) {
            strat_cfg.max_position = std::stoll(argv[++i]);
        } else if (std::strcmp(argv[i], "--base-latency") == 0 && i+1 < argc) {
            bt_cfg.latency_config.base_latency_ns = std::stoull(argv[++i]);
        } else if (std::strcmp(argv[i], "--verbose") == 0) {
            bt_cfg.verbose = true;
        } else if (std::strcmp(argv[i], "--help") == 0) {
            usage(argv[0]);
            return 0;
        } else {
            std::cerr << "unknown arg: " << argv[i] << "\n";
            usage(argv[0]);
            return 1;
        }
    }

    auto strategy = std::make_unique<hft::MeanReversionStrategy>(strat_cfg);

    hft::Backtester backtester(bt_cfg, std::move(strategy));
    auto results = backtester.run();

    hft::RiskMetrics::printReport(results);
    return 0;
}
