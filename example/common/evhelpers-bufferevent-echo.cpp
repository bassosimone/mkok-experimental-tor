// Part of measurement-kit <https://measurement-kit.github.io/>.
// Measurement-kit is free software. See AUTHORS and LICENSE for more
// information on the copying conditions.

#define EXAMPLE_UTIL_WANT_LISTEN_ONCE_AND_DISPATCH

#include <err.h>
#include <event2/bufferevent.h>
#include <event2/event.h>
#include <event2/util.h>
#include <functional>
#include <stddef.h>
#include <sys/time.h>
#include "src/common/evhelpers.hpp"

using namespace mk;

int main() {
    evhelpers::listen_once_and_dispatch([](Var<EventBase> base,
                                           evutil_socket_t conn) {
        timeval tv{7, 7};
        auto bev = Bufferevent::socket_new(base, conn, BEV_OPT_CLOSE_ON_FREE);
        evhelpers::sendrecv(bev, "", [base]() {
            evhelpers::break_soon(base);
        }, nullptr, &tv, true);
    });
}
