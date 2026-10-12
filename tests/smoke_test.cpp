#include "crypto/config.hpp"

#include <cassert>

int main() {
    const auto config = crypto::Config::defaults();
    config.validate();
    assert(config.symbols.size() == 5);
    assert(config.rolling_window == 1000);
    assert(config.beta == 1.0);
    return 0;
}