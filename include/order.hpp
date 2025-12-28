#pragma once
#include <cstdint>
#include <string>

enum class Side : std::uint8_t { Buy, Sell };

struct Order {
    std::uint64_t seq = 0;      // global sequence assigned at submit-time
    std::int64_t  ts = 0;       // synthetic timestamp (monotonic counter)
    std::string   symbol;
    Side          side = Side::Buy;
    std::int64_t  qty = 0;
    double        price = 0.0;
};

inline const char* side_str(Side s) {
    return s == Side::Buy ? "B" : "S";
}
