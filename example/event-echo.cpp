// Part of measurement-kit <https://measurement-kit.github.io/>.
// Measurement-kit is free software. See AUTHORS and LICENSE for more
// information on the copying conditions.

#include <err.h>
#include <event2/event.h>
#include <event2/util.h>
#include <memory>
#include <mkok/libevent.hpp>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

struct Context {
    std::string buffered;
    bool have_seen_eof = false;
    timeval timeout = {7, 7};
};

static void handle_io(Var<EventBase> base, evutil_socket_t conn,
                      Var<Context> ctx, short what) {
    char buff[1024];
    ssize_t count;
    int pending = 0;

    warnx("handle I/O");

    do {
        if ((what & EV_TIMEOUT) != 0) {
            warnx("timeout");
            break;
        }

        if ((what & EV_READ) != 0) {
            count = read(conn, buff, sizeof(buff));
            if (count < 0) {
                warn("read");
                break;
            }
            ctx->have_seen_eof = (count == 0);
            ctx->buffered += std::string(buff, count);
        }

        if (ctx->buffered.size() < 1048576 && !ctx->have_seen_eof) {
            warnx("will read more...");
            pending |= EV_READ;
        }

        if ((what & EV_WRITE) != 0) {
            count = write(conn, ctx->buffered.data(), ctx->buffered.size());
            if (count < 0) {
                warn("write");
                break;
            }
            ctx->buffered = ctx->buffered.substr(count);
        }

        if (ctx->buffered.size() > 0) {
            warnx("could write more...");
            pending |= EV_WRITE;
        }
    } while (0);

    if (pending == 0) {
        warnx("reached final state...");
        close(conn);
        return;
    }

    pending |= EV_TIMEOUT;
    EventBase::once(base, conn, pending, [base, conn, ctx](short what) {
        handle_io(base, conn, ctx, what);
    }, &ctx->timeout);
}

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

        handle_io(base, conn, std::make_shared<Context>(), 0);
    });

    // Dispatch I/O events

    warnx("loop...");
    EventBase::dispatch(base);
    warnx("loop... done");

    // Cleanup resources

    close(sock);
}
