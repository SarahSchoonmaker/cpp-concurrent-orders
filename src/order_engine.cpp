#include "order_engine.hpp"
#include <algorithm>

Order OrderEngine::assign_seq(Order o) {
    o.seq = next_seq_.fetch_add(1, std::memory_order_relaxed);
    {
        std::lock_guard<std::mutex> lk(mu_);
        submitted_++;
    }
    return o;
}

void OrderEngine::on_processed(Order o) {
    std::unique_lock<std::mutex> lk(mu_);
    processed_++;
    ready_.emplace(o.seq, std::move(o));
    try_commit_locked();
}

void OrderEngine::try_commit_locked() {
    while (true) {
        auto it = ready_.find(next_commit_seq_);
        if (it == ready_.end()) break;

        const Order& o = it->second;

        total_notional_ += static_cast<double>(o.qty) * o.price;

        auto& pos = net_pos_[o.symbol];
        if (o.side == Side::Buy) pos += o.qty;
        else pos -= o.qty;

        committed_.push_back(o);

        ready_.erase(it);
        next_commit_seq_++;
        cv_.notify_all();
    }
}

void OrderEngine::wait_until_committed(std::uint64_t last_seq) {
    std::unique_lock<std::mutex> lk(mu_);
    cv_.wait(lk, [&] { return next_commit_seq_ > last_seq; });
}

EngineSummary OrderEngine::summary() const {
    std::lock_guard<std::mutex> lk(mu_);
    EngineSummary s;
    s.submitted = submitted_;
    s.processed = processed_;
    s.committed = static_cast<std::uint64_t>(committed_.size());
    s.total_notional = total_notional_;
    s.net_position = net_pos_;
    return s;
}

std::vector<Order> OrderEngine::committed_sample(std::size_t n) const {
    std::lock_guard<std::mutex> lk(mu_);
    n = std::min(n, committed_.size());
    return std::vector<Order>(committed_.begin(), committed_.begin() + static_cast<std::ptrdiff_t>(n));
}
