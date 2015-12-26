// Part of measurement-kit <https://measurement-kit.github.io/>.
// Measurement-kit is free software. See AUTHORS and LICENSE for more
// information on the copying conditions.

#include <err.h>
#include <errno.h>
#include <event2/bufferevent.h>
#include <event2/bufferevent_ssl.h>
#include <event2/event.h>
#include <mkok/libevent.hpp>
#include <netinet/in.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <sys/time.h>
#include <unistd.h>

struct sockaddr;

static void print_errors(Var<Bufferevent> ssl_bev) {
    unsigned long error;
    char buffer[1024];
    for (;;) {
        error = Bufferevent::get_openssl_error(ssl_bev);
        if (error == 0) break;
        ERR_error_string_n(error, buffer, sizeof(buffer));
        warnx("ssl-err: %s", buffer);
    }
    warnx("socket-err: %s", strerror(errno));
}

#define USAGE "usage: %s [-A address] [-p port] [path]\n"

int main(int argc, char **argv) {
    std::string address = "127.0.0.1";
    std::string path = "/";
    std::string port = "4433";
    const char *progname = argv[0];
    int ch;

    while ((ch = getopt(argc, argv, "A:p:")) != -1) {
        switch (ch) {
        case 'A':
            address = optarg;
            break;
        case 'p':
            port = optarg;
            break;
        default:
            fprintf(stderr, USAGE, progname);
            exit(1);
        }
    }
    argc -= optind, argv += optind;
    if (argc != 0 && argc != 1) {
        fprintf(stderr, USAGE, progname);
        exit(1);
    }
    if (argc == 1) path = argv[0];

    // Initialize OpenSSL
    SSL_library_init();
    ERR_load_crypto_strings();
    SSL_load_error_strings();
    OpenSSL_add_all_algorithms();

    auto base = EventBase::create();
    auto bev = Bufferevent::socket_new(base, -1, BEV_OPT_CLOSE_ON_FREE);

    sockaddr_in sin;
    int len = sizeof(sin);
    Evutil::parse_sockaddr_port(address + ":" + port, (sockaddr *)&sin, &len);
    Bufferevent::socket_connect(bev, (sockaddr *)&sin, sizeof(sin));

    SSL_CTX *ssl_ctx = SSL_CTX_new(TLSv1_2_client_method());
    if (ssl_ctx == nullptr) err(1, "SSL_CTX_new");

    Bufferevent::setcb(bev, nullptr, nullptr, [base, bev, path,
                                               ssl_ctx](short what) {
        warnx("tcp %s", Bufferevent::event_string(what).c_str());
        if (what != BEV_EVENT_CONNECTED) {
            warnx("socket-err: %s", strerror(errno));
            // Remove self references
            Bufferevent::setcb(bev, nullptr, nullptr, nullptr);
            EventBase::loopbreak(base);
            return;
        }
        SSL *ssl = SSL_new(ssl_ctx);
        if (ssl == nullptr) err(1, "SSL_new");
        auto ssl_bev = Bufferevent::openssl_filter_new(
            base, bev, ssl, BUFFEREVENT_SSL_CONNECTING, BEV_OPT_CLOSE_ON_FREE);

        Bufferevent::setcb(ssl_bev, nullptr, nullptr, [base, path,
                                                       ssl_bev](short what) {
            warnx("ssl %s", Bufferevent::event_string(what).c_str());
            if (what != BEV_EVENT_CONNECTED) {
                print_errors(ssl_bev);
                // Remove self references
                Bufferevent::setcb(ssl_bev, nullptr, nullptr, nullptr);
                EventBase::loopbreak(base);
                return;
            }
            Bufferevent::enable(ssl_bev, EV_READ);
            std::string s = "GET " + path + "\r\n";
            Bufferevent::write(ssl_bev, s.data(), s.size());
            Bufferevent::setcb(
                ssl_bev,
                [ssl_bev]() {
                    for (;;) {
                        char b[1024];
                        size_t n = Bufferevent::read(ssl_bev, b, sizeof(b));
                        if (n <= 0) break;
                        ssize_t r = write(1, b, n);
                        if (r < 0 || (size_t)r != n) {
                            warn("Write error or short write occurred");
                        }
                    }
                },
                nullptr,
                [base, ssl_bev](short what) {
                    warnx("ssl* %s", Bufferevent::event_string(what).c_str());
                    print_errors(ssl_bev);
                    // Remove self references
                    Bufferevent::setcb(ssl_bev, nullptr, nullptr, nullptr);
                    EventBase::loopbreak(base);
                });
        });
    });

    timeval tv{7, 7};
    Bufferevent::set_timeouts(bev, &tv, &tv);

    warnx("loop...");
    EventBase::dispatch(base);
    warnx("loop... done");

    SSL_CTX_free(ssl_ctx);
}
