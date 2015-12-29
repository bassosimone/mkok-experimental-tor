// Part of measurement-kit <https://measurement-kit.github.io/>.
// Measurement-kit is free software. See AUTHORS and LICENSE for more
// information on the copying conditions.

#include <functional>
#include <iostream>
#include <mkok/libevent.hpp>
#include <string>
#include <sys/time.h>
#include "util.hpp"

int main() {
    example::util::VERBOSE = true;
    std::string out;
    std::string *outp = &out;
    auto base = EventBase::create();
    example::util::connect(
        base, "130.192.181.193:80", [base, outp](Var<Bufferevent> bev) {
            example::util::sendrecv(base, bev, "GET /\r\n", outp);
        });
    EventBase::dispatch(base);
    std::cout << out << std::endl;
}
