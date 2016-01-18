// Part of measurement-kit <https://measurement-kit.github.io/>.
// Measurement-kit is free software. See AUTHORS and LICENSE for more
// information on the copying conditions.

#define SRC_COMMON_LIBEVENT_EVUTIL_ENABLE_MOCK
#define CATCH_CONFIG_MAIN

#include "src/common/libevent-evutil.hpp"
#include "catch.hpp"

using namespace mk;

TEST_CASE("We deal with evutil_make_socket_nonblocking success") {
    EvutilMock mock;
    mock.evutil_make_socket_nonblocking = [](evutil_socket_t) { return 0; };
    evutil::make_socket_nonblocking(&mock, 0);
}

TEST_CASE("We deal with evutil_make_socket_nonblocking failure") {
    EvutilMock mock;
    mock.evutil_make_socket_nonblocking = [](evutil_socket_t) { return -1; };
    REQUIRE_THROWS_AS(evutil::make_socket_nonblocking(&mock, 0),
                      EvutilMakeSocketNonblockingError);
}

TEST_CASE("We deal with evutil_parse_sockaddr_port success") {
    EvutilMock mock;
    mock.evutil_parse_sockaddr_port = [](const char *, sockaddr *, int *) {
        return 0;
    };
    REQUIRE(evutil::parse_sockaddr_port(&mock, "", nullptr, nullptr) == 0);
}

TEST_CASE("We deal with evutil_parse_sockaddr_port failure") {
    EvutilMock mock;
    mock.evutil_parse_sockaddr_port = [](const char *, sockaddr *, int *) {
        return -1;
    };
    REQUIRE(evutil::parse_sockaddr_port(&mock, "", nullptr, nullptr) ==
            EvutilParseSockaddrPortError());
}

TEST_CASE("We deal with evutil_make_listen_socket_reuseable success") {
    EvutilMock mock;
    mock.evutil_make_listen_socket_reuseable = [](evutil_socket_t) {
        return 0;
    };
    evutil::make_listen_socket_reuseable(&mock, 0);
}

TEST_CASE("We deal with evutil_make_listen_socket_reuseable failure") {
    EvutilMock mock;
    mock.evutil_make_listen_socket_reuseable = [](evutil_socket_t) {
        return -1;
    };
    REQUIRE_THROWS_AS(evutil::make_listen_socket_reuseable(&mock, 0),
                      EvutilMakeListenSocketReuseableError);
}
