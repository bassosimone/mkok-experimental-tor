// Part of measurement-kit <https://measurement-kit.github.io/>.
// Measurement-kit is free software. See AUTHORS and LICENSE for more
// information on the copying conditions.

#ifndef SRC_COMMON_LIBEVENT_EVUTIL_HPP
#define SRC_COMMON_LIBEVENT_EVUTIL_HPP

#include <event2/util.h>
#include <functional>
#include <measurement_kit/common/error.hpp>
#include <string>
struct sockaddr;

namespace mk {
namespace evutil {

template <int (*func)(evutil_socket_t) = ::evutil_make_socket_nonblocking>
void make_socket_nonblocking(evutil_socket_t s) {
    if (func(s) != 0) {
        MK_THROW(EvutilMakeSocketNonblockingError);
    }
}

template <int (*func)(const char *, sockaddr *,
                      int *) = ::evutil_parse_sockaddr_port>
Error __attribute__((warn_unused_result))
parse_sockaddr_port(std::string s, sockaddr *p, int *n) {
    if (func(s.c_str(), p, n) != 0) {
        return EvutilParseSockaddrPortError();
    }
    return NoError();
}

template <int (*func)(evutil_socket_t) = ::evutil_make_listen_socket_reuseable>
void make_listen_socket_reuseable(evutil_socket_t s) {
    if (func(s) != 0) {
        MK_THROW(EvutilMakeListenSocketReuseableError);
    }
}

} // namespace evutil
} // namespace mk
#endif
