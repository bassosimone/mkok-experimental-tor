// Part of measurement-kit <https://measurement-kit.github.io/>.
// Measurement-kit is free software. See AUTHORS and LICENSE for more
// information on the copying conditions.

#include <functional>
#include <iostream>
#include <string>
#include <sys/time.h>
#include "src/tor/onion-poller.hpp"
#include "src/net/socks.hpp"
#include "src/common/evhelpers.hpp"

using namespace mk;

int main() {
    static const char *request = "GET /robots.txt\r\n";
    evhelpers::VERBOSE = true;
    std::string out;
    std::string *outp = &out;
    OnionPoller::loop([outp](Var<OnionPoller> poller) {
        OnionPoller::enable_tor(poller, [outp, poller](OnionStatus status) {
            std::cout << "enable... " << (int)status << "\n";
            if (status != OnionStatus::OK) {
                OnionPoller::break_soon(poller);
                return;
            }
            std::cout << "connecting to socks proxy...\n";
            Socks::connect(
                poller->evbase, "130.192.16.172", 80,
                [outp, poller](SocksStatus status, Var<Bufferevent> bev) {
                    std::cout << "proxy connect... " << (int)status << "\n";
                    if (status != SocksStatus::OK) {
                        OnionPoller::break_soon(poller);
                        return;
                    }
                    evhelpers::sendrecv(bev, request, [outp, poller]() {
                        OnionPoller::disable_tor(
                            poller, [poller](OnionStatus status) {
                                std::cout << "disable: " << (int)status << "\n";
                                OnionPoller::break_soon( poller);
                            });
                    }, outp);
                });
        }, 5);
    });
    std::cout << out << std::endl;
}
