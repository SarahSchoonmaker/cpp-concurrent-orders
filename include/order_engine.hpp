#pragma once
#include "order.hpp"
#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <map>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

struct EngineSummary {
    std::uint64_t submitted = 0;
    std::uint64_t processed = 0; // processed by workers
    std::uint64_t committed = 0; // committed in sequence order
    double total_notional = 0.0;

    // Deterministic ordering for printing
    std::map<std::string, std::int64_t> net_position;
};

class OrderEngine {
public:
    OrderEngine() = default;

    // Assigns global seq at submit-time
    Order assign_seq(Order o);

    // Called by workers after processing
    void on_processed(Order o);

    // Wait until all orders up to last_seq committed
    void wait_until_committed(std::uint64_t last_seq);

    EngineSummary summary() const;
    std::vector<Order> committed_sample(std::size_t n) const;

private:
    void try_commit_locked();

    mutable std::mutex mu_;
    std::condition_variable cv_;

    std::atomic<std::uint64_t> next_seq_{1};
    std::uint64_t submitted_ = 0;
    std::uint64_t processed_ = 0;

    std::uint64_t next_commit_seq_ = 1;
    std::unordered_map<std::uint64_t, Order> ready_;

    double total_notional_ = 0.0;
    std::map<std::string, std::int64_t> net_pos_;
    std::vector<Order> committed_;
};
