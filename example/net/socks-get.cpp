// Part of measurement-kit <https://measurement-kit.github.io/>.
// Measurement-kit is free software. See AUTHORS and LICENSE for more
// information on the copying conditions.

#include <functional>
#include <iostream>
#include <string>
#include <sys/time.h>
#include "src/net/socks.hpp"
#include "src/common/evhelpers.hpp"

using namespace mk;

int main() {
    evhelpers::VERBOSE = true;
    std::string out;
    std::string *outp = &out;
    auto base = EventBase::create();
    Socks::connect(base, "130.192.16.172", 80,
        [base, outp](SocksStatus status, Var<Bufferevent> bev) {
            std::cout << "status: " << (int)status << "\n";
            if (status != SocksStatus::OK) {
                evhelpers::break_soon(base);
                return;
            }
            evhelpers::sendrecv(bev, "GET /robots.txt\r\n", [base]() {
                evhelpers::break_soon(base);
            }, outp);
        });
    base->dispatch();
    std::cout << out << std::endl;
}
