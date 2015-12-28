// Part of measurement-kit <https://measurement-kit.github.io/>.
// Measurement-kit is free software. See AUTHORS and LICENSE for more
// information on the copying conditions.

#define CATCH_CONFIG_MAIN

#include <mkok/libevent.hpp>
#include <openssl/conf.h>
#include <openssl/engine.h>
#include <openssl/err.h>
#include "test/catch.hpp"

static const int kFlags = BEV_OPT_CLOSE_ON_FREE;

static void connect(Var<EventBase> evbase, const char *endpoint, bool *isconn,
                    std::function<void(Var<Bufferevent>)> callback) {
    Var<Bufferevent> bev = Bufferevent::socket_new(evbase, -1, kFlags);
    sockaddr_in sin;
    int len = sizeof (sin);
    sockaddr *sa = (sockaddr *) &sin;
    Evutil::parse_sockaddr_port(endpoint, sa, &len);
    Bufferevent::socket_connect(bev, sa, len);
    timeval timeo;
    timeo.tv_usec = timeo.tv_sec = 3;
    Bufferevent::set_timeouts(bev, &timeo, &timeo);
    Bufferevent::setcb(bev, nullptr, nullptr,
        [bev, callback, evbase, isconn](short what) {
            if (what != BEV_EVENT_CONNECTED) {
                // Clear self reference
                Bufferevent::setcb(bev, nullptr, nullptr, nullptr);
                EventBase::loopbreak(evbase);
                return;
            }
            *isconn = true;
            callback(bev);
        });
}

static void sendrecv(Var<EventBase> evbase, Var<Bufferevent> bev,
                     std::string request, std::string *output) {
    Bufferevent::write(bev, request.data(), request.size());
    Bufferevent::enable(bev, EV_READ);
    Bufferevent::setcb(bev, [bev, output]() {
        Var<Evbuffer> evbuf = Evbuffer::create();
        Bufferevent::read_buffer(bev, evbuf);
        std::string s = Evbuffer::pullup(evbuf, -1);
        *output += s;
    }, nullptr, [bev, evbase](short) {
        // Clear self reference
        Bufferevent::setcb(bev, nullptr, nullptr, nullptr);
        EventBase::loopbreak(evbase);
        return;
    });
}

static const char *kRequest = "GET /robots.txt HTTP/1.0\r\n\r\n";

TEST_CASE("Retrieve HTTP resource using bufferevent") {
    bool connected = false;
    std::string output;
    std::string *po = &output;
    Var<EventBase> evbase = EventBase::create();
    connect(evbase, "130.192.16.172:80", &connected,
        [evbase, po](Var<Bufferevent> bev) {
            sendrecv(evbase, bev, kRequest, po);
        });
    EventBase::dispatch(evbase);
    REQUIRE(connected == true);
    REQUIRE(output != "");
}

TEST_CASE("Retrieve HTTPS resource using bufferevent") {
    SSL_library_init();
    ERR_load_crypto_strings();
    SSL_load_error_strings();
    OpenSSL_add_all_algorithms();
    SSL_CTX *ctx = SSL_CTX_new(SSLv23_client_method());
    REQUIRE(ctx != nullptr);

    bool connected = false;
    bool ssl_conn = false;
    bool *pssl_conn = &ssl_conn;
    std::string output;
    std::string *po = &output;

    Var<EventBase> evbase = EventBase::create();
    connect(evbase, "38.229.72.16:443", &connected,
        [ctx, evbase, po, pssl_conn](Var<Bufferevent> bev) {
            SSL *secure = SSL_new(ctx);
            REQUIRE(secure != nullptr);
            auto bev2 = Bufferevent::openssl_filter_new(
                evbase, bev, secure, BUFFEREVENT_SSL_CONNECTING, kFlags);
            Bufferevent::setcb(bev2, nullptr, nullptr,
                [evbase, bev2, po, pssl_conn](short what) {
                    if (what != BEV_EVENT_CONNECTED) {
                        // Clear self reference
                        Bufferevent::setcb(bev2, nullptr, nullptr, nullptr);
                        EventBase::loopbreak(evbase);
                        return;
                    }
                    *pssl_conn = true;
                    sendrecv(evbase, bev2, kRequest, po);
                });
        });
    EventBase::dispatch(evbase);

    REQUIRE(connected == true);
    REQUIRE(ssl_conn == true);
    REQUIRE(output != "");
    SSL_CTX_free(ctx);

    CONF_modules_free();
    ERR_remove_state(0);
    ENGINE_cleanup();
    CONF_modules_unload(1);
    ERR_free_strings();
    EVP_cleanup();
    CRYPTO_cleanup_all_ex_data();
}
