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

#ifdef SRC_COMMON_LIBEVENT_EVUTIL_ENABLE_MOCK

class EvutilMock {
  public:
#define XX(name, signature) std::function<signature> name = ::name
    XX(evutil_make_listen_socket_reuseable, int(evutil_socket_t));
    XX(evutil_make_socket_nonblocking, int(evutil_socket_t));
    XX(evutil_parse_sockaddr_port, int(const char *, sockaddr *, int *));
#undef XX
};

#define MockPtrArg EvutilMock *mockp,
#define call(func, ...) mockp->func(__VA_ARGS__)

#else

#define MockPtrArg
#define call(func, ...) ::func(__VA_ARGS__)

#endif

namespace evutil {

inline void make_socket_nonblocking(MockPtrArg evutil_socket_t s) {
    if (call(evutil_make_socket_nonblocking, s) != 0) {
        MK_THROW(EvutilMakeSocketNonblockingError);
    }
}

inline Error __attribute__((warn_unused_result))
parse_sockaddr_port(MockPtrArg std::string s, sockaddr *p, int *n) {
    if (call(evutil_parse_sockaddr_port, s.c_str(), p, n) != 0) {
        return EvutilParseSockaddrPortError();
    }
    return NoError();
}

inline void make_listen_socket_reuseable(MockPtrArg evutil_socket_t s) {
    if (call(evutil_make_listen_socket_reuseable, s) != 0) {
        MK_THROW(EvutilMakeListenSocketReuseableError);
    }
}

} // namespace evutil

// Undefine internal macros
#undef MockPtrArg
#undef call

} // namespace mk
#endif
