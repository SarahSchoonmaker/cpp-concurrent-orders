#pragma once
#include "bounded_queue.hpp"
#include <atomic>
#include <functional>
#include <thread>
#include <vector>

// Minimal thread pool consuming std::function<void()> tasks from a bounded queue.
// Focus: correctness + clean shutdown.
class ThreadPool {
public:
    ThreadPool(std::size_t workers, std::size_t queue_capacity)
        : tasks_(queue_capacity) {
        start(workers);
    }

    ~ThreadPool() {
        stop();
    }

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    bool submit(std::function<void()> fn) {
        if (stopped_.load()) return false;
        return tasks_.push(std::move(fn));
    }

    void stop() {
        bool expected = false;
        if (!stopped_.compare_exchange_strong(expected, true)) return;
        tasks_.close();
        for (auto& t : threads_) if (t.joinable()) t.join();
    }

private:
    void start(std::size_t workers) {
        threads_.reserve(workers);
        for (std::size_t i = 0; i < workers; i++) {
            threads_.emplace_back([this] {
                while (true) {
                    auto task = tasks_.pop();
                    if (!task.has_value()) break;
                    (*task)();
                }
            });
        }
    }

    BoundedQueue<std::function<void()>> tasks_;
    std::vector<std::thread> threads_;
    std::atomic<bool> stopped_{false};
};
