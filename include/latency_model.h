#pragma once

#include "types.h"
#include <random>
#include <cstdint>

namespace hft {

// Simulates network + exchange latency with gaussian and uniform jitter.
class LatencyModel {
public:
    struct Config {
        uint64_t base_latency_ns = 50'000;      // 50us wire
        uint64_t exchange_latency_ns = 10'000;   // 10us exch processing
        double gaussian_jitter_std_ns = 5'000.0; // 5us std dev
        uint64_t uniform_jitter_max_ns = 3'000;  // 3us bounded noise
        uint64_t seed = 42;
    };

    LatencyModel();
    explicit LatencyModel(const Config& config);

    uint64_t simulateLatency();
    void applyLatency(ExecutionReport& report);

    const Config& config() const { return config_; }

private:
    Config config_;
    std::mt19937_64 rng_;
    std::normal_distribution<double> gaussian_;
    std::uniform_int_distribution<uint64_t> uniform_;
};

} // namespace hft
