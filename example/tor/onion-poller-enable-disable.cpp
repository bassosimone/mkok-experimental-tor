// Part of measurement-kit <https://measurement-kit.github.io/>.
// Measurement-kit is free software. See AUTHORS and LICENSE for more
// information on the copying conditions.

#include <functional>
#include <iostream>
#include "src/common/libevent-core.hpp"
#include "src/tor/onion-poller.hpp"

using namespace mk;

int main() {
    OnionPoller::loop([](Var<OnionPoller> poller) {
        static const unsigned timeout = 5;
        std::cout << "enable tor...\n";
        OnionPoller::enable_tor(poller, [poller](OnionStatus status) {
            std::cout << "enable tor... " << (int)status << "\n";
            if (status != OnionStatus::OK) {
                OnionPoller::break_loop(poller);
                return;
            }
            std::cout << "disable tor...\n";
            OnionPoller::disable_tor(poller, [poller](OnionStatus status) {
                std::cout << "disable tor... " << (int)status << "\n";
                OnionPoller::break_loop(poller);
            });
        }, timeout);
    });
};
