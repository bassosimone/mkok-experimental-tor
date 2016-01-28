// Part of measurement-kit <https://measurement-kit.github.io/>.
// Measurement-kit is free software. See AUTHORS and LICENSE for more
// information on the copying conditions.

#include <functional>
#include "src/common/libevent.hpp"
#include "src/tor/onion-poller.hpp"

using namespace mk;

int main() {
    OnionPoller::loop(
        [](Var<OnionPoller> poller) { OnionPoller::break_loop(poller); });
};
