#include "csv_parser.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>

namespace hft {

CSVParser::CSVParser(const std::string& filepath)
    : filepath_(filepath) {}

size_t CSVParser::parse(const std::function<void(const MarketUpdate&)>& callback) {
    std::ifstream file(filepath_);
    if (!file.is_open()) {
        std::cerr << "csv_parser: can't open " << filepath_ << "\n";
        return 0;
    }

    std::string line;
    size_t count = 0;

    // skip header row
    if (!std::getline(file, line))
        return 0;

    MarketUpdate update;
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        if (parseLine(line, update)) {
            callback(update);
            ++count;
        }
    }
    return count;
}

std::vector<MarketUpdate> CSVParser::parseAll() {
    std::vector<MarketUpdate> updates;
    updates.reserve(4096);
    parse([&](const MarketUpdate& u) { updates.push_back(u); });
    return updates;
}

bool CSVParser::parseLine(const std::string& line, MarketUpdate& update) {
    std::vector<std::string> tokens;
    tokens.reserve(8);

    size_t start = 0, end = 0;
    while ((end = line.find(',', start)) != std::string::npos) {
        tokens.emplace_back(line.substr(start, end - start));
        start = end + 1;
    }
    tokens.emplace_back(line.substr(start));

    if (tokens.size() < 8)
        return false;

    // trim whitespace
    for (auto& t : tokens) {
        t.erase(0, t.find_first_not_of(" \t\r\n"));
        t.erase(t.find_last_not_of(" \t\r\n") + 1);
    }

    try {
        update.timestamp = std::stoull(tokens[0]);
        update.setSymbol(tokens[1]);
        update.bid_price = std::stod(tokens[2]);
        update.ask_price = std::stod(tokens[3]);
        update.bid_qty = static_cast<uint32_t>(std::stoul(tokens[4]));
        update.ask_qty = static_cast<uint32_t>(std::stoul(tokens[5]));
        update.last_price = std::stod(tokens[6]);
        update.last_qty = static_cast<uint32_t>(std::stoul(tokens[7]));
    } catch (const std::exception& e) {
        std::cerr << "csv_parser: bad line: " << line << " (" << e.what() << ")\n";
        return false;
    }
    return true;
}

} // namespace hft
