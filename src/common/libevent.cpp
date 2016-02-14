// Part of measurement-kit <https://measurement-kit.github.io/>.
// Measurement-kit is free software. See AUTHORS and LICENSE for more
// information on the copying conditions.

#include <measurement_kit/common/func.hpp>
#include <measurement_kit/common/var.hpp>
#include <sys/time.h>
#include "src/common/libevent.hpp"

extern "C" {

void mk_libevent_event_cb(evutil_socket_t, short w, void *p) {
    auto f = static_cast<std::function<void(short)> *>(p);
    (*f)(w);
    delete f;
}

static void is_ignored_by_cxx(mk::Var<mk::Bufferevent> *varp) {
    // If a smart pointer is ignored by C++ code, we can dispose of it.
    // If `bevp` is not null, we put it into a state in which it cannot hurt,
    // otherwise, if it's null, it was stolen by SSL code.
    // We do not delete the pointer immediately because the underlying code
    // that called us may not like it; instead postpone to next cycle.
    // Calling delete is going to trigger bufferevent_free().
    if ((*varp)->bevp != nullptr) {
        ::bufferevent_setcb((*varp)->bevp, nullptr, nullptr, nullptr, nullptr);
        (void)::bufferevent_disable((*varp)->bevp, EV_READ | EV_WRITE);
    }
    timeval to;
    to.tv_sec = to.tv_usec = 0;
    (*varp)->evbase->once(-1, EV_TIMEOUT, [varp](short) { delete varp; }, &to);
}

void mk_libevent_bev_read(bufferevent *, void *ptr) {
    auto varp = static_cast<mk::Var<mk::Bufferevent> *>(ptr);
    // Only callback if C++ code is still interested to Bufferevent
    if (!varp->unique() && (*varp)->read_cb) (*varp)->read_cb();
    // After the callback, check whether the C++ code is still interested
    // to use this Bufferevent, otherwise "garbage collect" it
    if (varp->unique()) is_ignored_by_cxx(varp);
}

void mk_libevent_bev_write(bufferevent *, void *ptr) {
    auto varp = static_cast<mk::Var<mk::Bufferevent> *>(ptr);
    if (!varp->unique() && (*varp)->write_cb) (*varp)->write_cb();
    if (varp->unique()) is_ignored_by_cxx(varp);
}

void mk_libevent_bev_event(bufferevent *, short what, void *ptr) {
    auto varp = static_cast<mk::Var<mk::Bufferevent> *>(ptr);
    if (!varp->unique() && (*varp)->event_cb) (*varp)->event_cb(what);
    if (varp->unique()) is_ignored_by_cxx(varp);
}

void handle_resolve(int code, char type, int count, int ttl, void *addresses,
                    void *opaque) {
    mk::EvdnsBase::EvdnsCallback *callback =
        static_cast<mk::EvdnsBase::EvdnsCallback *>(opaque);
    (*callback)(code, type, count, ttl, addresses);
    delete callback;
}

} // extern "C"
