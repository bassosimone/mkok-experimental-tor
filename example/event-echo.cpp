// Part of measurement-kit <https://measurement-kit.github.io/>.
// Measurement-kit is free software. See AUTHORS and LICENSE for more
// information on the copying conditions.

#define EXAMPLE_UTIL_WANT_LISTEN_ONCE_AND_DISPATCH

#include <err.h>
#include <event2/event.h>
#include <event2/util.h>
#include <functional>
#include <memory>
#include <mkok/libevent.hpp>
#include <string>
#include <sys/time.h>
#include <unistd.h>
#include "util.hpp"

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
    example::util::listen_once_and_dispatch(
        [](Var<EventBase> base, evutil_socket_t conn) {
            handle_io(base, conn, std::make_shared<Context>(), 0);
        });
}
