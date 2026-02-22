#pragma once

#include "types.h"
#include <string>
#include <vector>
#include <functional>

namespace hft {

// Streams a CSV file and emits MarketUpdate objects.
// Expected format: timestamp,symbol,bid_price,ask_price,bid_qty,ask_qty,last_price,last_qty
class CSVParser {
public:
    explicit CSVParser(const std::string& filepath);
    ~CSVParser() = default;

    size_t parse(const std::function<void(const MarketUpdate&)>& callback);
    std::vector<MarketUpdate> parseAll();

    const std::string& filepath() const { return filepath_; }

private:
    std::string filepath_;
    bool parseLine(const std::string& line, MarketUpdate& update);
};

} // namespace hft
