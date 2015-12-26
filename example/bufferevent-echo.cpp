// Part of measurement-kit <https://measurement-kit.github.io/>.
// Measurement-kit is free software. See AUTHORS and LICENSE for more
// information on the copying conditions.

#include <err.h>
#include <event2/bufferevent.h>
#include <event2/event.h>
#include <event2/util.h>
#include <mkok/libevent.hpp>
#include <netinet/in.h>
#include <stddef.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

int main() {

    // Create listening socket

    evutil_socket_t sock;
    sockaddr_in sin;
    sockaddr *sa = (sockaddr *)&sin;
    int len = sizeof(sin);

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) err(1, "socket");
    Evutil::make_listen_socket_reuseable(sock);
    Evutil::parse_sockaddr_port("0.0.0.0:54321", sa, &len);
    if (bind(sock, sa, len)) err(1, "bind");
    if (listen(sock, 17)) err(1, "listen");
    Evutil::make_socket_nonblocking(sock);

    warnx("listening...");

    // Prepare to receive a single ACCEPT event

    Var<EventBase> base = EventBase::create();
    EventBase::once(base, sock, EV_READ, [sock, base](short) {
        warnx("accept...");
        evutil_socket_t conn = accept(sock, nullptr, nullptr);
        if (conn < 0) {
            warn("accept");
            return;
        }
        Evutil::make_socket_nonblocking(conn);

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

    // Dispatch I/O events

    warnx("loop...");
    EventBase::dispatch(base);
    warnx("loop... done");

    // Cleanup resources

    close(sock);
}
