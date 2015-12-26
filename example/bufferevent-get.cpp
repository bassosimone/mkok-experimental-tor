// Part of measurement-kit <https://measurement-kit.github.io/>.
// Measurement-kit is free software. See AUTHORS and LICENSE for more
// information on the copying conditions.

#include <err.h>
#include <event2/bufferevent.h>
#include <event2/event.h>
#include <mkok/libevent.hpp>
#include <netinet/in.h>
#include <stddef.h>
#include <string>
#include <sys/time.h>
#include <unistd.h>

struct sockaddr;

int main() {
    auto base = EventBase::create();
    auto bev = Bufferevent::socket_new(base, -1, BEV_OPT_CLOSE_ON_FREE);

    sockaddr_in sin;
    int len = sizeof(sin);
    Evutil::parse_sockaddr_port("130.192.181.193:80", (sockaddr *)&sin, &len);
    Bufferevent::socket_connect(bev, (sockaddr *)&sin, sizeof(sin));

    Bufferevent::setcb(bev, nullptr, nullptr, [base, bev](short what) {
        warnx("%s", Bufferevent::event_string(what).c_str());
        if (what != BEV_EVENT_CONNECTED) {
            EventBase::loopbreak(base);
            return;
        }
        Bufferevent::enable(bev, EV_READ);
        std::string s = "GET /\r\n";
        Bufferevent::write(bev, s.data(), s.size());
        Bufferevent::setcb(
            bev,
            [bev]() {
                for (;;) {
                    char b[1024];
                    size_t n = Bufferevent::read(bev, b, sizeof(b));
                    if (n <= 0) break;
                    ssize_t r = write(1, b, n);
                    if (r < 0 || (size_t)r != n) {
                        warn("Write error or short write occurred");
                    }
                }
            },
            nullptr,
            [base, bev](short what) {
                warnx("%s", Bufferevent::event_string(what).c_str());
                // Remove self references
                Bufferevent::setcb(bev, nullptr, nullptr, nullptr);
                EventBase::loopbreak(base);
            });
    });

    timeval tv{7, 7};
    Bufferevent::set_timeouts(bev, &tv, &tv);

    warnx("loop...");
    EventBase::dispatch(base);
    warnx("loop... done");
}
