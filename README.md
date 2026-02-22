# HFT Backtesting Engine

A C++17 backtesting engine for simulating high-frequency trading strategies. Built as a personal project to explore order book mechanics, low-latency patterns, and quantitative risk analysis.

## What it does

- Reads tick-level market data from CSV files
- Maintains a price-time priority order book
- Matches limit and market orders with realistic latency simulation
- Runs a mean-reversion strategy (z-score based) as a demo
- Reports risk metrics: Sharpe, Sortino, max drawdown, win rate, profit factor

## How it works

The engine uses a multi-threaded pipeline where a data feed thread pushes market updates through a lock-free SPSC queue to the strategy thread. The strategy thread runs the order book, matching engine, and portfolio tracking inline. Latency is modeled with configurable base delay + gaussian/uniform jitter.

The lock-free queue uses cache-line padding and acquire/release memory ordering to avoid false sharing between threads.

## Building

You need a C++17 compiler (g++ or clang++).

```bash
cd hft_backtesting_engine
mkdir -p build

# with cmake (if you have it)
cd build && cmake .. && make -j4

# or just compile directly
g++ -std=c++17 -O2 -Iinclude -pthread src/*.cpp -o build/hft_backtest
```

## Running

```bash
./build/hft_backtest data/sample_market_data.csv
```

Add `--verbose` to see order book snapshots and individual fills:

```bash
./build/hft_backtest data/sample_market_data.csv --verbose
```

### Options

| Flag | Description | Default |
|------|-------------|---------|
| `--capital` | Starting cash | 1000000 |
| `--window` | Rolling lookback window (ticks) | 50 |
| `--k-factor` | Z-score threshold for signals | 1.5 |
| `--order-qty` | Shares per trade | 100 |
| `--max-position` | Max net position (risk limit) | 500 |
| `--base-latency` | Simulated wire latency in ns | 50000 |
| `--verbose` | Print book + fill details | off |

## Project structure

```
include/        headers
  types.h             core types (Order, MarketUpdate, ExecutionReport, etc.)
  spsc_queue.h        lock-free single-producer/single-consumer ring buffer
  csv_parser.h        market data CSV reader
  order_book.h        price-time priority order book
  matching_engine.h   limit/market order matching
  latency_model.h     network + exchange latency simulation
  strategy.h          abstract strategy interface
  mean_reversion_strategy.h
  backtester.h        main backtest driver + portfolio tracker
  risk_metrics.h      sharpe, sortino, drawdown, etc.

src/            implementations
data/           sample tick data (500 rows of synthetic AAPL)
```

## Using your own data

The CSV format is:

```
timestamp,symbol,bid_price,ask_price,bid_qty,ask_qty,last_price,last_qty
```

Timestamp should be nanoseconds (uint64). Drop your file in `data/` and point the binary at it.

## Writing a new strategy

Implement the `IStrategy` interface in `include/strategy.h`:

```cpp
class MyStrategy : public hft::IStrategy {
public:
    std::vector<hft::Order> onMarketUpdate(const hft::MarketUpdate& update) override;
    void onExecutionReport(const hft::ExecutionReport& report) override;
    const char* name() const override { return "MyStrategy"; }
};
```

Then plug it into `main.cpp` instead of the mean reversion strategy.

## Sample output

```
=== HFT Backtesting Engine ===
  data:     data/sample_market_data.csv
  capital:  $1000000
  latency:  50us base + 10us exchange
  strategy: MeanReversion
==============================

Backtest done in 0ms | 500 ticks | 60 orders | 60 fills | $889374.00 volume

========== RISK METRICS ==========
  Total PnL:        $3639.0000
  Sharpe:           0.2902
  Sortino:          0.4213
  Max Drawdown:     0.59%
  Win Rate:         57.14%
  Profit Factor:    4.2085
==================================
```
