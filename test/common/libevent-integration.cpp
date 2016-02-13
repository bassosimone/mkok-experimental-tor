// Part of measurement-kit <https://measurement-kit.github.io/>.
// Measurement-kit is free software. See AUTHORS and LICENSE for more
// information on the copying conditions.

#define CATCH_CONFIG_MAIN

#include "src/common/evhelpers.hpp"
#include "catch.hpp"

using namespace mk;

static const char *REQUEST = "GET /robots.txt HTTP/1.0\r\n\r\n";

TEST_CASE("Retrieve HTTP resource using bufferevent") {
    bool connected = false;
    std::string output;
    std::string *po = &output;
    Var<EventBase> evbase = EventBase::create();
    evhelpers::connect(evbase,
                       "130.192.16.172:80", [evbase, po](Var<Bufferevent> bev) {
                           evhelpers::sendrecv(bev, REQUEST, [evbase]() {
                               evhelpers::break_soon(evbase);
                           }, po);
                       }, &connected);
    evbase->dispatch();
    REQUIRE(connected == true);
    REQUIRE(output != "");
}

TEST_CASE("Connect to closed port using bufferevent") {
    bool connected = false;
    std::string output;
    std::string *po = &output;
    Var<EventBase> evbase = EventBase::create();
    evhelpers::connect(evbase, "130.192.91.211:88", [evbase](Var<Bufferevent>) {
        evbase->loopbreak();
    }, &connected);
    evbase->dispatch();
    REQUIRE(output == "");
    REQUIRE(connected == false);
}

TEST_CASE("Retrieve HTTPS resource using bufferevent") {
    bool connected = false;
    bool ssl_connected = false;
    std::string output;
    std::string *po = &output;

    Var<EventBase> evbase = EventBase::create();
    evhelpers::ssl_connect(
        evbase, "38.229.72.16:443",
        evhelpers::SslContext::get(), [evbase, po](Var<Bufferevent> bev) {
            evhelpers::sendrecv(bev, REQUEST, [evbase]() {
                evhelpers::break_soon(evbase);
            }, po);
        }, &connected, &ssl_connected);
    evbase->dispatch();

    REQUIRE(connected == true);
    REQUIRE(ssl_connected == true);
    REQUIRE(output != "");
}

TEST_CASE("Connect to port where SSL is not active") {
    bool connected = false;
    bool ssl_connected = false;
    std::string output;
    std::string *po = &output;

    Var<EventBase> evbase = EventBase::create();
    evhelpers::ssl_connect(evbase, "130.192.16.172:80",
                           evhelpers::SslContext::get(),
                           [evbase](Var<Bufferevent>) { evbase->loopbreak(); },
                           &connected, &ssl_connected);
    evbase->dispatch();

    REQUIRE(connected == true);
    REQUIRE(ssl_connected == false);
    REQUIRE(output == "");
}

TEST_CASE("event_base_once works") {
    bool called = false;
    timeval timeo;
    timeo.tv_sec = 1;
    timeo.tv_usec = 17;
    Var<EventBase> evbase = EventBase::create();
    evbase->once(-1, EV_TIMEOUT, [&called, evbase](short w) {
        REQUIRE(w == EV_TIMEOUT);
        evbase->loopbreak();
        called = true;
    }, &timeo);
    evbase->dispatch();
    REQUIRE(called == true);
}

TEST_CASE("Resolve ipv4 works") {
    Var<EventBase> evbase = EventBase::create();
    Var<EvdnsBase> dnsbase = EvdnsBase::create(evbase,true, true);
    REQUIRE(dnsbase != nullptr);
    EvdnsBase::resolve_ipv4 (dnsbase, "nexa.polito.it", 
                        [evbase](int result, char type, int count, int ttl, 
                            std::vector<std::string> addresses) {
                            REQUIRE(result == DNS_ERR_NONE);
                            REQUIRE(addresses[0]=="130.192.16.172");
                            event_base_loopbreak(evbase->evbase);
                        });
    event_base_dispatch(evbase->evbase);
}
TEST_CASE("Resolve ipv6 works") {
    Var<EventBase> evbase = EventBase::create();
    Var<EvdnsBase> dnsbase = EvdnsBase::create(evbase);
    REQUIRE(dnsbase != nullptr);
    EvdnsBase::resolve_ipv6 (dnsbase, "google.com",
                        [evbase](int result, char type, int count, int ttl,
                            std::vector<std::string> addresses) {
                            REQUIRE(result == DNS_ERR_NONE);
                            REQUIRE(addresses[0]=="2a00:1450:4001:807::200e");
                            event_base_loopbreak(evbase->evbase);
                        });
    event_base_dispatch(evbase->evbase);
}
