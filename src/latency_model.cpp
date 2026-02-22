#include "latency_model.h"
#include <algorithm>

namespace hft {

LatencyModel::LatencyModel() : LatencyModel(Config{}) {}

LatencyModel::LatencyModel(const Config& config)
    : config_(config),
      rng_(config.seed),
      gaussian_(0.0, config.gaussian_jitter_std_ns),
      uniform_(0, config.uniform_jitter_max_ns) {}

uint64_t LatencyModel::simulateLatency() {
    double lat = static_cast<double>(config_.base_latency_ns);
    lat += gaussian_(rng_);  // can go slightly negative, that's fine
    lat += static_cast<double>(uniform_(rng_));
    lat += static_cast<double>(config_.exchange_latency_ns);
    return static_cast<uint64_t>(std::max(lat, 0.0));
}

void LatencyModel::applyLatency(ExecutionReport& report) {
    report.receive_ts = report.exchange_ts + simulateLatency();
}

} // namespace hft
