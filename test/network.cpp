// Part of measurement-kit <https://measurement-kit.github.io/>.
// Measurement-kit is free software. See AUTHORS and LICENSE for more
// information on the copying conditions.

#define CATCH_CONFIG_MAIN

#include "example/util.hpp"
#include "test/catch.hpp"

static const char *kRequest = "GET /robots.txt HTTP/1.0\r\n\r\n";

TEST_CASE("Retrieve HTTP resource using bufferevent") {
    bool connected = false;
    std::string output;
    std::string *po = &output;
    Var<EventBase> evbase = EventBase::create();
    example::util::connect(
        evbase, "130.192.16.172:80", [evbase, po](Var<Bufferevent> bev) {
            example::util::sendrecv(evbase, bev, kRequest, po);
        }, &connected);
    EventBase::dispatch(evbase);
    REQUIRE(connected == true);
    REQUIRE(output != "");
}

TEST_CASE("Connect to closed port using bufferevent") {
    bool connected = false;
    std::string output;
    std::string *po = &output;
    Var<EventBase> evbase = EventBase::create();
    example::util::connect(evbase,
                           "130.192.91.211:88", [evbase](Var<Bufferevent>) {
                               EventBase::loopbreak(evbase);
                           }, &connected);
    EventBase::dispatch(evbase);
    REQUIRE(output == "");
    REQUIRE(connected == false);
}

TEST_CASE("Retrieve HTTPS resource using bufferevent") {
    bool connected = false;
    bool ssl_connected = false;
    std::string output;
    std::string *po = &output;

    Var<EventBase> evbase = EventBase::create();
    example::util::ssl_connect(
        evbase, "130.192.16.172:443",
        example::util::SslContext::get(), [evbase, po](Var<Bufferevent> bev) {
            example::util::sendrecv(evbase, bev, kRequest, po);
        }, &connected, &ssl_connected);
    EventBase::dispatch(evbase);

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
    example::util::ssl_connect(
        evbase, "130.192.16.172:80", example::util::SslContext::get(),
        [evbase](Var<Bufferevent>) { EventBase::loopbreak(evbase); },
        &connected, &ssl_connected);
    EventBase::dispatch(evbase);

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
    EventBase::once(evbase, -1, EV_TIMEOUT, [&called, evbase](short w) {
        REQUIRE(w == EV_TIMEOUT);
        EventBase::loopbreak(evbase);
        called = true;
    }, &timeo);
    EventBase::dispatch(evbase);
    REQUIRE(called == true);
}
