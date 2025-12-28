#include "order_engine.hpp"
#include <cassert>
#include <iostream>

int main() {
    OrderEngine eng;

    Order a; a.symbol="SYM0"; a.side=Side::Buy;  a.qty=10; a.price=100.0;
    Order b; b.symbol="SYM0"; b.side=Side::Sell; b.qty=3;  b.price=100.0;
    Order c; c.symbol="SYM0"; c.side=Side::Buy;  c.qty=1;  c.price=100.0;

    a = eng.assign_seq(a);
    b = eng.assign_seq(b);
    c = eng.assign_seq(c);

    // processed out of order, but committed in seq order
    eng.on_processed(b);
    eng.on_processed(a);
    eng.on_processed(c);

    eng.wait_until_committed(3);

    auto s = eng.summary();
    assert(s.committed == 3);
    assert(s.net_position.at("SYM0") == 8);
    assert(static_cast<long long>(s.total_notional) == 1400);

    std::cout << "determinism ok\n";
    return 0;
}
