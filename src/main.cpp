#include "order_engine.hpp"
#include "thread_pool.hpp"
#include <chrono>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <random>
#include <string>
#include <thread>
#include <vector>

struct Args {
    std::uint64_t orders = 20000;
    std::size_t producers = 4;
    std::size_t workers = 4;
    std::size_t symbols = 25;
    std::size_t queue_cap = 8192;
    std::uint64_t seed = 42;
    bool print_sample = false;
};

static void usage(const char* prog) {
    std::cerr << "Usage: " << prog
              << " [--orders N] [--producers N] [--workers N] [--symbols N] [--queue N] [--seed N] [--print-sample]\n";
}

static bool parse_u64(const char* s, std::uint64_t& out) {
    if (!s || !*s) return false;
    char* endp = nullptr;
    auto v = std::strtoull(s, &endp, 10);
    if (!endp || *endp != '\0') return false;
    out = static_cast<std::uint64_t>(v);
    return true;
}

static bool parse_size(const char* s, std::size_t& out) {
    std::uint64_t tmp = 0;
    if (!parse_u64(s, tmp)) return false;
    out = static_cast<std::size_t>(tmp);
    return true;
}

static std::vector<std::string> make_symbols(std::size_t n) {
    std::vector<std::string> syms;
    syms.reserve(n);
    for (std::size_t i = 0; i < n; i++) syms.push_back("SYM" + std::to_string(i));
    return syms;
}

int main(int argc, char** argv) {
    Args a;

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        auto need = [&](const char* name) -> const char* {
            if (i + 1 >= argc) { std::cerr << "Missing value for " << name << "\n"; usage(argv[0]); std::exit(2); }
            return argv[++i];
        };

        if (arg == "--orders") parse_u64(need("--orders"), a.orders);
        else if (arg == "--producers") parse_size(need("--producers"), a.producers);
        else if (arg == "--workers") parse_size(need("--workers"), a.workers);
        else if (arg == "--symbols") parse_size(need("--symbols"), a.symbols);
        else if (arg == "--queue") parse_size(need("--queue"), a.queue_cap);
        else if (arg == "--seed") parse_u64(need("--seed"), a.seed);
        else if (arg == "--print-sample") a.print_sample = true;
        else if (arg == "--help" || arg == "-h") { usage(argv[0]); return 0; }
        else { std::cerr << "Unknown arg: " << arg << "\n"; usage(argv[0]); return 2; }
    }

    const auto symbols = make_symbols(a.symbols);
    ThreadPool pool(a.workers, a.queue_cap);
    OrderEngine engine;

    std::atomic<std::uint64_t> ts_counter{1};
    std::atomic<std::uint64_t> submitted{0};

    auto start = std::chrono::steady_clock::now();

    std::vector<std::thread> producers;
    producers.reserve(a.producers);

    const std::uint64_t per = a.orders / a.producers;
    const std::uint64_t rem = a.orders % a.producers;

    for (std::size_t p = 0; p < a.producers; p++) {
        const std::uint64_t count = per + (p < rem ? 1 : 0);
        producers.emplace_back([&, p, count] {
            std::mt19937_64 rng(a.seed + static_cast<std::uint64_t>(p));
            std::uniform_int_distribution<std::size_t> sym_dist(0, symbols.size() - 1);
            std::uniform_int_distribution<int> side_dist(0, 1);
            std::uniform_int_distribution<int> qty_dist(1, 250);
            std::uniform_real_distribution<double> px_dist(10.0, 500.0);

            for (std::uint64_t i = 0; i < count; i++) {
                Order o;
                o.ts = static_cast<std::int64_t>(ts_counter.fetch_add(1, std::memory_order_relaxed));
                o.symbol = symbols[sym_dist(rng)];
                o.side = (side_dist(rng) == 0) ? Side::Buy : Side::Sell;
                o.qty = qty_dist(rng);
                o.price = px_dist(rng);

                o = engine.assign_seq(std::move(o));
                submitted.fetch_add(1, std::memory_order_relaxed);

                bool ok = pool.submit([&, o = std::move(o)]() mutable {
                    // Tiny CPU work simulation
                    volatile double x = 0.0;
                    for (int k = 0; k < 50; k++) x += k * 0.00001;
                    (void)x;

                    engine.on_processed(std::move(o));
                });
                if (!ok) break;
            }
        });
    }

    for (auto& t : producers) t.join();
    pool.stop();

    const std::uint64_t last_seq = submitted.load(std::memory_order_relaxed);
    engine.wait_until_committed(last_seq);

    auto end = std::chrono::steady_clock::now();
    const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    const auto s = engine.summary();

    std::cout << "RUN SUMMARY\n-----------\n";
    std::cout << "Orders submitted:  " << s.submitted << "\n";
    std::cout << "Orders processed:  " << s.processed << "\n";
    std::cout << "Orders committed:  " << s.committed << "\n";
    std::cout << "Total notional:    " << std::fixed << std::setprecision(2) << s.total_notional << "\n";
    std::cout << "Elapsed (ms):      " << ms << "\n\n";

    std::cout << "NET POSITION (first 10 symbols)\n-------------------------------\n";
    int shown = 0;
    for (const auto& kv : s.net_position) {
        std::cout << kv.first << "  " << kv.second << "\n";
        if (++shown >= 10) break;
    }

    if (a.print_sample) {
        auto sample = engine.committed_sample(10);
        std::cout << "\nCOMMITTED SAMPLE (first 10)\n--------------------------\n";
        for (const auto& o : sample) {
            std::cout << "#" << o.seq << " ts=" << o.ts << " " << o.symbol
                      << " " << side_str(o.side) << " qty=" << o.qty
                      << " px=" << std::fixed << std::setprecision(2) << o.price << "\n";
        }
    }

    return 0;
}
