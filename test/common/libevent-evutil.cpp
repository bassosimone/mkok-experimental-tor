// Part of measurement-kit <https://measurement-kit.github.io/>.
// Measurement-kit is free software. See AUTHORS and LICENSE for more
// information on the copying conditions.

#define SRC_COMMON_LIBEVENT_EVUTIL_ENABLE_MOCK
#define CATCH_CONFIG_MAIN

#include "src/common/libevent-evutil.hpp"
#include "catch.hpp"

using namespace mk;

static int success(evutil_socket_t) { return 0; }
static int fail(evutil_socket_t) { return -1; }

TEST_CASE("We deal with evutil_make_socket_nonblocking success") {
    evutil::make_socket_nonblocking<success>(0);
}

TEST_CASE("We deal with evutil_make_socket_nonblocking failure") {
    REQUIRE_THROWS_AS(evutil::make_socket_nonblocking<fail>(0),
                      EvutilMakeSocketNonblockingError);
}

static int success(const char *, sockaddr *, int *) { return 0; }
static int fail(const char *, sockaddr *, int *) { return -1; }

TEST_CASE("We deal with evutil_parse_sockaddr_port success") {
    REQUIRE(evutil::parse_sockaddr_port<success>("", nullptr, nullptr) == 0);
}

TEST_CASE("We deal with evutil_parse_sockaddr_port failure") {
    REQUIRE(evutil::parse_sockaddr_port<fail>("", nullptr, nullptr) ==
            EvutilParseSockaddrPortError());
}

TEST_CASE("We deal with evutil_make_listen_socket_reuseable success") {
    evutil::make_listen_socket_reuseable<success>(0);
}

TEST_CASE("We deal with evutil_make_listen_socket_reuseable failure") {
    REQUIRE_THROWS_AS(evutil::make_listen_socket_reuseable<fail>(0),
                      EvutilMakeListenSocketReuseableError);
}
