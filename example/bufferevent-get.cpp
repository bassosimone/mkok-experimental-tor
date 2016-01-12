// Part of measurement-kit <https://measurement-kit.github.io/>.
// Measurement-kit is free software. See AUTHORS and LICENSE for more
// information on the copying conditions.

#include <functional>
#include <iostream>
#include <mkok/libevent.hpp>
#include <mkok/evhelpers.hpp>
#include <string>
#include <sys/time.h>

int main() {
    evhelpers::VERBOSE = true;
    std::string out;
    std::string *outp = &out;
    auto base = EventBase::create();
    evhelpers::connect(
        base, "130.192.181.193:80", [base, outp](Var<Bufferevent> bev) {
            evhelpers::sendrecv(bev, "GET /\r\n", [base]() {
                evhelpers::break_soon(base);
            }, outp);
        });
    EventBase::dispatch(base);
    std::cout << out << std::endl;
}
