// Part of measurement-kit <https://measurement-kit.github.io/>.
// Measurement-kit is free software. See AUTHORS and LICENSE for more
// information on the copying conditions.

#ifndef SRC_COMMON_EVHELPERS_HPP
#define SRC_COMMON_EVHELPERS_HPP

#include <err.h>
#include <errno.h>
#include <event2/bufferevent.h>
#include <event2/bufferevent_ssl.h>
#include <event2/event.h>
#include <event2/util.h>
#include <exception>
#include <functional>
#include <iostream>
#include <measurement_kit/common/var.hpp>
#include <netinet/in.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <string.h>
#include <string>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
#include "src/common/libevent.hpp"

namespace mk {
namespace evhelpers {

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
        if ((error = bev->get_openssl_error()) == 0) break;
        ERR_error_string_n(error, buffer, sizeof(buffer));
        if (VERBOSE) {
            std::clog << "ssl: " << buffer << "\n";
        }
    }
}

/// Will break soon out of the main loop. The rationale for not breaking out
/// immediately is that I've recently committed code for delaying the cleanup
/// of bufferevents. This works by registering a dead bufferevent to be
/// deleted in the next I/O loop cycle. And is motivated by the fact that
/// destroying a bufferevent immediately leads to user after free at least
/// with SSL bufferevents. Hence this function to wait a little bit just
/// before actually exiting, to avoid valgrind false positives.
/// \param evbase The event base.
void break_soon(Var<EventBase> evbase);

void break_soon(Var<EventBase> evbase) {
    timeval timeo;
    timeo.tv_sec = timeo.tv_usec = 2;
    evbase->once(-1, EV_TIMEOUT, [evbase](short) {
        evbase->loopbreak();
    }, &timeo);
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
    if (evutil::parse_sockaddr_port(endpoint, sa, &len) != 0) {
        std::clog << "Cannot parse sockaddr_port\n";
        break_soon(evbase);
        return;
    }
    bev->socket_connect(sa, len);
    timeval timeo;
    timeo.tv_usec = timeo.tv_sec = 3;
    bev->set_timeouts(&timeo, &timeo);
    bev->setcb(nullptr, nullptr,
                       [bev, callback, evbase, isconnected](short what) {
                           if (what != BEV_EVENT_CONNECTED) {
                               possibly_print_error(what, bev);
                               // Clear self reference
                               bev->setcb(nullptr, nullptr,
                                                  nullptr);
                               break_soon(evbase);
                               return;
                           }
                           if (isconnected) *isconnected = true;
                           callback(bev);
                       });
}

void sendrecv(Var<Bufferevent> bev, std::string request,
              std::function<void()> cb, std::string *output = nullptr,
              const timeval *timeout = nullptr, bool must_echo = false);

void sendrecv(Var<Bufferevent> bev, std::string request,
              std::function<void()> cb, std::string *output,
              const timeval *timeout, bool must_echo) {
    if (request != "") {
        bev->write(request.data(), request.size());
    }
    if (timeout) bev->set_timeouts(timeout, timeout);
    bev->enable(EV_READ);
    bev->setcb(
        [bev, must_echo, output]() {
            Var<Evbuffer> evbuf = Evbuffer::create();
            bev->read_buffer(evbuf);
            if (output) {
                *output += evbuf->remove(evbuf->get_length());
            } else if (must_echo) {
                bev->write_buffer(evbuf);
            }
        },
        []() { /* Note to self: this is here to increase coverage */ },
        [bev, cb, output](short what) {
            possibly_print_error(what, bev);

            Var<Evbuffer> input = bev->get_input();
            if (input->get_length() > 0) {
                // We still have some more buffered data to add to output
                *output += input->remove(input->get_length());
            }

            Var<Evbuffer> output = bev->get_output();
            if (output->get_length() > 0) {
                // If we still have data to write, we must wait for the output
                // buffer to empty before we can close the connection
                bev->setcb(
                    nullptr,
                    [bev, cb]() {
                        Var<Evbuffer> output = bev->get_output();
                        if (output->get_length() > 0) {
                            return; // I think this should not happen
                        }
                        // Clear self reference
                        bev->setcb(nullptr, nullptr, nullptr);
                        cb();
                    },
                    [bev, cb](short what) {
                        // My understanding is that we will land here in
                        // case there is a write timeout and in this case
                        // probably the best thing is to close
                        possibly_print_error(what, bev);
                        // Clear self reference
                        bev->setcb(nullptr, nullptr, nullptr);
                        cb();
                    });
                return;
            }

            // Clear self reference
            bev->setcb(nullptr, nullptr, nullptr);
            cb();
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
                ssl_bev->setcb(
                    nullptr, nullptr,
                    [callback, evbase, ssl_bev, ssl_isconnected](short what) {
                        if (what != BEV_EVENT_CONNECTED) {
                            possibly_print_error(what, ssl_bev);
                            // Clear self reference
                            ssl_bev->setcb(nullptr, nullptr, nullptr);
                            break_soon(evbase);
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
    evutil::make_listen_socket_reuseable(sock);
    if (evutil::parse_sockaddr_port("0.0.0.0:54321", sa, &len) != 0) {
        err(1, "parse_sockaddr_port");
    }
    if (bind(sock, sa, len)) err(1, "bind");
    if (listen(sock, 17)) err(1, "listen");
    evutil::make_socket_nonblocking(sock);

    warnx("listening...");

    // Prepare to receive a single ACCEPT event

    Var<EventBase> base = EventBase::create();
    base->once(sock, EV_READ, [base, callback, sock](short) {
        warnx("accept...");
        evutil_socket_t conn = accept(sock, nullptr, nullptr);
        if (conn < 0) {
            warn("accept");
            return;
        }
        evutil::make_socket_nonblocking(conn);

        // Deal with I/O events on `conn`

        callback(base, conn);
    });

    // Dispatch I/O events

    warnx("loop...");
    base->dispatch();
    warnx("loop... done");

    // Cleanup resources

    close(sock);
}

} // namespace evhelpers
} // namespace mk
#endif
