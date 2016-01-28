// Part of measurement-kit <https://measurement-kit.github.io/>.
// Measurement-kit is free software. See AUTHORS and LICENSE for more
// information on the copying conditions.

#define CATCH_CONFIG_MAIN

#include <functional>
#include "src/common/libevent.hpp"
#include "src/tor/onion-poller.hpp"
#include "catch.hpp"

/* Note: one test case per file because Tor is not actually a library
   that you can call, shutdown, and call again. There is global stuff in
   there that it does not like it when you treat it that way. */

using namespace mk;

TEST_CASE("We can enable and then suddenly disable tor and leave loop") {
    static const unsigned timeout = 5;
    OnionPoller::loop([](Var<OnionPoller> poller) {
        OnionPoller::enable_tor(poller, [poller](OnionStatus status) {
            if (status != OnionStatus::OK) {
                OnionPoller::break_loop(poller);
                return;
            }
            OnionPoller::disable_tor(poller, [poller](OnionStatus /*status*/) {
                OnionPoller::break_loop(poller);
            });
        }, timeout);
    });
}
