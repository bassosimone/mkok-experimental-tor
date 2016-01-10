// Part of measurement-kit <https://measurement-kit.github.io/>.
// Measurement-kit is free software. See AUTHORS and LICENSE for more
// information on the copying conditions.
#ifndef EXAMPLE_UTIL_HPP
#define EXAMPLE_UTIL_HPP

#include <err.h>
#include <errno.h>
#include <event2/bufferevent.h>
#include <event2/bufferevent_ssl.h>
#include <event2/event.h>
#include <event2/util.h>
#include <exception>
#include <functional>
#include <iostream>
#include <mkok/libevent.hpp>
#include <mkok/evutil.hpp>
#include <netinet/in.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <string.h>
#include <string>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

namespace example {
namespace util {

static const int FLAGS = BEV_OPT_CLOSE_ON_FREE;
static bool VERBOSE = false;

static void possibly_print_error(short what, Var<Bufferevent> bev) {
    // We could bail out immediately if not verbose, but in this case it
    // makes sense to run more code to increase coverage
    std::string descr = Bufferevent::event_string(what);
    if (VERBOSE) {
        std::clog << "tcp: " << descr << "\n";
    }
    if ((what & BEV_EVENT_ERROR) != 0 && VERBOSE) {
        std::clog << "errno: " << strerror(errno) << "\n";
    }
    unsigned long error;
    char buffer[1024];
    for (;;) {
        if ((error = Bufferevent::get_openssl_error(bev)) == 0) break;
        ERR_error_string_n(error, buffer, sizeof(buffer));
        if (VERBOSE) {
            std::clog << "ssl: " << buffer << "\n";
        }
    }
}

void connect(Var<EventBase> evbase, const char *endpoint,
             std::function<void(Var<Bufferevent>)> callback,
             bool *isconnected = nullptr);

void connect(Var<EventBase> evbase, const char *endpoint,
             std::function<void(Var<Bufferevent>)> callback,
             bool *isconnected) {
    Var<Bufferevent> bev = Bufferevent::socket_new(evbase, -1, FLAGS);
    sockaddr_in sin;
    int len = sizeof(sin);
    sockaddr *sa = (sockaddr *)&sin;
    Evutil::parse_sockaddr_port(endpoint, sa, &len);
    Bufferevent::socket_connect(bev, sa, len);
    timeval timeo;
    timeo.tv_usec = timeo.tv_sec = 3;
    Bufferevent::set_timeouts(bev, &timeo, &timeo);
    Bufferevent::setcb(bev, nullptr, nullptr,
                       [bev, callback, evbase, isconnected](short what) {
                           if (what != BEV_EVENT_CONNECTED) {
                               possibly_print_error(what, bev);
                               // Clear self reference
                               Bufferevent::setcb(bev, nullptr, nullptr,
                                                  nullptr);
                               EventBase::loopbreak(evbase);
                               return;
                           }
                           if (isconnected) *isconnected = true;
                           callback(bev);
                       });
}

void sendrecv(Var<EventBase> evbase, Var<Bufferevent> bev, std::string request,
              std::string *output = nullptr, const timeval *timeout = nullptr,
              bool must_echo = false);

void sendrecv(Var<EventBase> evbase, Var<Bufferevent> bev, std::string request,
              std::string *output, const timeval *timeout, bool must_echo) {
    if (request != "") {
        Bufferevent::write(bev, request.data(), request.size());
    }
    if (timeout) Bufferevent::set_timeouts(bev, timeout, timeout);
    Bufferevent::enable(bev, EV_READ);
    Bufferevent::setcb(
        bev,
        [bev, must_echo, output]() {
            Var<Evbuffer> evbuf = Evbuffer::create();
            Bufferevent::read_buffer(bev, evbuf);
            if (output) {
                *output += Evbuffer::pullup(evbuf, -1);
            } else if (must_echo) {
                Bufferevent::write_buffer(bev, evbuf);
            }
        },
        []() { /* Note to self: this is here to increase coverage */ },
        [bev, evbase, output](short what) {
            possibly_print_error(what, bev);

            Var<Evbuffer> input = Bufferevent::get_input(bev);
            if (Evbuffer::get_length(input) > 0) {
                // We still have some more buffered data to add to output
                *output += Evbuffer::pullup(input, -1);
                Evbuffer::drain(input, Evbuffer::get_length(input));
            }

            Var<Evbuffer> output = Bufferevent::get_output(bev);
            if (Evbuffer::get_length(output) > 0) {
                // If we still have data to write, we must wait for the output
                // buffer to empty before we can close the connection
                Bufferevent::setcb(
                    bev, nullptr,
                    [bev, evbase]() {
                        Var<Evbuffer> output = Bufferevent::get_output(bev);
                        if (Evbuffer::get_length(output) > 0) {
                            return; // I think this should not happen
                        }
                        // Clear self reference
                        Bufferevent::setcb(bev, nullptr, nullptr, nullptr);
                        EventBase::loopbreak(evbase);
                    },
                    [bev, evbase](short what) {
                        // My understanding is that we will land here in
                        // case there is a write timeout and in this case
                        // probably the best thing is to close
                        possibly_print_error(what, bev);
                        // Clear self reference
                        Bufferevent::setcb(bev, nullptr, nullptr, nullptr);
                        EventBase::loopbreak(evbase);
                    });
                return;
            }

            // Clear self reference
            Bufferevent::setcb(bev, nullptr, nullptr, nullptr);
            EventBase::loopbreak(evbase);
            return;
        });
}

void ssl_connect(Var<EventBase> evbase, const char *endpoint, SSL_CTX *context,
                 std::function<void(Var<Bufferevent>)> callback,
                 bool *isconnected = nullptr, bool *ssl_isconnected = nullptr);

void ssl_connect(Var<EventBase> evbase, const char *endpoint, SSL_CTX *context,
                 std::function<void(Var<Bufferevent>)> callback,
                 bool *isconnected, bool *ssl_isconnected) {
    connect(evbase, endpoint,
            [callback, context, evbase, ssl_isconnected](Var<Bufferevent> bev) {
                SSL *secure = SSL_new(context);
                if (secure == nullptr) {
                    throw std::exception();
                }
                auto ssl_bev = Bufferevent::openssl_filter_new(
                    evbase, bev, secure, BUFFEREVENT_SSL_CONNECTING, FLAGS);
                Bufferevent::setcb(
                    ssl_bev, nullptr, nullptr,
                    [callback, evbase, ssl_bev, ssl_isconnected](short what) {
                        if (what != BEV_EVENT_CONNECTED) {
                            possibly_print_error(what, ssl_bev);
                            // Clear self reference
                            Bufferevent::setcb(ssl_bev, nullptr, nullptr,
                                               nullptr);
                            EventBase::loopbreak(evbase);
                            return;
                        }
                        if (ssl_isconnected) *ssl_isconnected = true;
                        callback(ssl_bev);
                    });
            },
            isconnected);
}

class SslContext {
  public:
    static SSL_CTX *get() {
        static SslContext singleton;
        return singleton.ctx;
    }

  private:
    SslContext() {
        SSL_library_init();
        ERR_load_crypto_strings();
        SSL_load_error_strings();
        OpenSSL_add_all_algorithms();

        // Note: the code in here is really simple because we only aim to
        // smoke test the OpenSSL functionality. What is missing here is at
        // the minimum code to verify the peer certificate. Also we should
        // not allow SSLv2 (and probably v3) connections, and we should also
        // probably set SNI for the other end to respond correctly.
        ctx = SSL_CTX_new(SSLv23_client_method());
        if (ctx == nullptr) {
            throw std::exception();
        }
    }

    ~SslContext() { SSL_CTX_free(ctx); }

    SSL_CTX *ctx = nullptr;
};

#ifdef EXAMPLE_UTIL_WANT_LISTEN_ONCE_AND_DISPATCH
void listen_once_and_dispatch(
    std::function<void(Var<EventBase>, evutil_socket_t)> callback);

void listen_once_and_dispatch(
    std::function<void(Var<EventBase>, evutil_socket_t)> callback) {

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
    EventBase::once(base, sock, EV_READ, [base, callback, sock](short) {
        warnx("accept...");
        evutil_socket_t conn = accept(sock, nullptr, nullptr);
        if (conn < 0) {
            warn("accept");
            return;
        }
        Evutil::make_socket_nonblocking(conn);

        // Deal with I/O events on `conn`

        callback(base, conn);
    });

    // Dispatch I/O events

    warnx("loop...");
    EventBase::dispatch(base);
    warnx("loop... done");

    // Cleanup resources

    close(sock);
}
#endif // EXAMPLE_UTIL_WANT_LISTEN_ONCE_AND_DISPATCH

} // namespace util
} // namespace example
#endif
