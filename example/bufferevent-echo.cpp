// Part of measurement-kit <https://measurement-kit.github.io/>.
// Measurement-kit is free software. See AUTHORS and LICENSE for more
// information on the copying conditions.

#include <err.h>
#include <event2/bufferevent.h>
#include <event2/event.h>
#include <event2/util.h>
#include <functional>
#include <mkok/libevent.hpp>
#include <stddef.h>
#include <sys/time.h>
#include "util.hpp"

int main() {
    example::util::listen_once_and_dispatch([](Var<EventBase> base,
                                               evutil_socket_t conn) {

        // Deal with I/O events on `conn`

        auto bev = Bufferevent::socket_new(base, conn, BEV_OPT_CLOSE_ON_FREE);
        Bufferevent::setcb(
            bev,
            [bev]() {
                warnx("readable...");
                char b[1024];
                size_t n;
                while ((n = Bufferevent::read(bev, b, sizeof(b))) > 0) {
                    Bufferevent::write(bev, b, n);
                }
                warnx("readable... ok");
            },
            nullptr,
            [bev](short what) {
                warnx("event: %s", Bufferevent::event_string(what).c_str());
                // Remove self references
                Bufferevent::setcb(bev, nullptr, nullptr, nullptr);
            });
        timeval tv{7, 7};
        Bufferevent::set_timeouts(bev, &tv, &tv);
        Bufferevent::enable(bev, EV_READ);
    });
}
