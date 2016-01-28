// Part of measurement-kit <https://measurement-kit.github.io/>.
// Measurement-kit is free software. See AUTHORS and LICENSE for more
// information on the copying conditions.

#ifndef SRC_COMMON_LIBEVENT_CORE_HPP
#define SRC_COMMON_LIBEVENT_CORE_HPP

#include <cstddef>
#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <event2/bufferevent_ssl.h>
#include <event2/event.h>
#include <event2/util.h>
#include <exception>
#include <functional>
#include <memory>
#include <measurement_kit/common/error.hpp>
#include <measurement_kit/common/func.hpp>
#include <measurement_kit/common/var.hpp>
#include <string>
#include <utility>
#include "src/common/libevent-evbuffer.hpp"

// Forward declarations
struct bufferevent;
struct evbuffer;
struct event_base;
struct sockaddr;
struct ssl_st;
struct timeval;

extern "C" {

// Using C linkage for C callbacks is recommended by C++ FAQ
//  see <https://isocpp.org/wiki/faq/pointers-to-members#memfnptr-vs-fnptr>

void mk_libevent_bev_read(bufferevent *, void *ptr);
void mk_libevent_bev_write(bufferevent *, void *ptr);
void mk_libevent_bev_event(bufferevent *, short what, void *ptr);
void mk_libevent_event_cb(evutil_socket_t, short w, void *p);

} // extern "C"

namespace mk {

class EventBase {
  public:
    event_base *evbase = nullptr;
    bool owned = false;

    EventBase() {}
    EventBase(EventBase &) = delete;
    EventBase &operator=(EventBase &) = delete;
    EventBase(EventBase &&) = delete;
    EventBase &operator=(EventBase &&) = delete;
    ~EventBase() {}

    template <void (*func)(event_base *) = ::event_base_free>
    static Var<EventBase> assign(event_base *pointer, bool owned) {
        if (pointer == nullptr) {
            MK_THROW(NullPointerError);
        }
        Var<EventBase> base(new EventBase, [](EventBase *ptr) {
            if (ptr->owned && ptr->evbase != nullptr) {
                func(ptr->evbase);
                ptr->evbase = nullptr;
                ptr->owned = false;
            }
            delete ptr;
        });
        base->evbase = pointer;
        base->owned = owned;
        return base;
    }

    template <event_base *(*construct)() = ::event_base_new,
              void (*destruct)(event_base *) = ::event_base_free>
    static Var<EventBase> create() {
        return assign<destruct>(construct(), true);
    }

    template <int (*func)(event_base *) = ::event_base_dispatch>
    int dispatch() {
        int ctrl = func(evbase);
        if (ctrl != 0 && ctrl != 1) {
            MK_THROW(EventBaseDispatchError);
        }
        return ctrl;
    }

    template <int (*func)(event_base *, int) = ::event_base_loop>
    int loop(int flags) {
        int ctrl = func(evbase, flags);
        if (ctrl != 0 && ctrl != 1) {
            MK_THROW(EventBaseLoopError);
        }
        return ctrl;
    }

    template <int (*func)(event_base *) = ::event_base_loopbreak>
    void loopbreak() {
        if (func(evbase) != 0) {
            MK_THROW(EventBaseLoopbreakError);
        }
    }

    template <int (*func)(event_base *, evutil_socket_t, short,
                          event_callback_fn, void *,
                          const timeval *) = ::event_base_once>
    void once(evutil_socket_t sock, short what, std::function<void(short)> cb,
              const timeval *timeo = nullptr) {
        auto cbp = new std::function<void(short)>(cb);
        if (func(evbase, sock, what, mk_libevent_event_cb, cbp, timeo) != 0) {
            delete cbp;
            MK_THROW(EventBaseOnceError);
        }
    }
};

class Bufferevent {
  public:
    Func<void(short)> event_cb;
    Func<void()> read_cb;
    Func<void()> write_cb;
    bufferevent *bevp = nullptr;
    Var<EventBase> evbase;
#ifdef SRC_COMMON_LIBEVENT_CORE_ENABLE_MOCK
    Var<Bufferevent> *the_opaque = nullptr; // To avoid leak in regress tests
#endif

    Bufferevent() {}
    Bufferevent(Bufferevent &) = delete;
    Bufferevent &operator=(Bufferevent &) = delete;
    Bufferevent(Bufferevent &&) = delete;
    Bufferevent &operator=(Bufferevent &&) = delete;
    ~Bufferevent() {}

    static std::string event_string(short what) {
        std::string descr = "";
        if ((what & BEV_EVENT_READING) != 0) descr += "reading ";
        if ((what & BEV_EVENT_WRITING) != 0) descr += "writing ";
        if ((what & BEV_EVENT_CONNECTED) != 0) descr += "connected ";
        if ((what & BEV_EVENT_EOF) != 0) descr += "eof ";
        if ((what & BEV_EVENT_TIMEOUT) != 0) descr += "timeout ";
        if ((what & BEV_EVENT_ERROR) != 0) descr += "error ";
        return descr;
    }

    template <bufferevent *(*construct)(event_base *, evutil_socket_t,
                                        int) = ::bufferevent_socket_new,
              void (*destruct)(bufferevent *) = ::bufferevent_free>
    static Var<Bufferevent> socket_new(Var<EventBase> base, evutil_socket_t fd,
                                       int flags) {
        Bufferevent *ptr = new Bufferevent;
        ptr->evbase = base;
        ptr->bevp = construct(base->evbase, fd, flags);
        if (ptr->bevp == nullptr) {
            delete ptr;
            MK_THROW(BuffereventSocketNewError);
        }
        Var<Bufferevent> *varp = new Var<Bufferevent>(ptr, [](Bufferevent *p) {
            if (p->bevp != nullptr) {
                destruct(p->bevp);
                p->bevp = nullptr;
            }
            delete p;
        });
#ifdef SRC_COMMON_LIBEVENT_CORE_ENABLE_MOCK
        ptr->the_opaque = varp;
#endif
        // We pass `varp` so C code keeps us alive
        bufferevent_setcb(ptr->bevp, mk_libevent_bev_read,
                          mk_libevent_bev_write, mk_libevent_bev_event, varp);
        return *varp;
    }

    template <int (*func)(bufferevent *, sockaddr *,
                          int) = ::bufferevent_socket_connect>
    void socket_connect(sockaddr *sa, int len) {
        if (func(bevp, sa, len) != 0) {
            MK_THROW(BuffereventSocketConnectError);
        }
    }

    void setcb(std::function<void()> readcb, std::function<void()> writecb,
               std::function<void(short)> eventcb) {
        event_cb = eventcb;
        read_cb = readcb;
        write_cb = writecb;
    }

    template <int (*func)(bufferevent *, const void *,
                          size_t) = ::bufferevent_write>
    void write(const void *base, size_t count) {
        if (func(bevp, base, count) != 0) {
            MK_THROW(BuffereventWriteError);
        }
    }

    template <int (*func)(bufferevent *,
                          evbuffer *) = ::bufferevent_write_buffer>
    void write_buffer(Var<Evbuffer> s) {
        if (func(bevp, s->evbuf) != 0) {
            MK_THROW(BuffereventWriteBufferError);
        }
    }

    size_t read(void *base, size_t count) {
        return ::bufferevent_read(bevp, base, count);
    }

    template <int (*func)(bufferevent *,
                          evbuffer *) = ::bufferevent_read_buffer>
    void read_buffer(Var<Evbuffer> d) {
        if (func(bevp, d->evbuf) != 0) {
            MK_THROW(BuffereventReadBufferError);
        }
    }

    template <int (*func)(bufferevent *, short) = ::bufferevent_enable>
    void enable(short what) {
        if (func(bevp, what) != 0) {
            MK_THROW(BuffereventEnableError);
        }
    }

    template <int (*func)(bufferevent *, short) = ::bufferevent_disable>
    void disable(short what) {
        if (func(bevp, what) != 0) {
            MK_THROW(BuffereventDisableError);
        }
    }

    template <int (*func)(bufferevent *, const timeval *,
                          const timeval *) = ::bufferevent_set_timeouts>
    void set_timeouts(const timeval *rto, const timeval *wto) {
        if (func(bevp, rto, wto) != 0) {
            MK_THROW(BuffereventSetTimeoutsError);
        }
    }

    template <bufferevent *(*construct)(event_base *, bufferevent *, ssl_st *,
                                        bufferevent_ssl_state,
                                        int) = ::bufferevent_openssl_filter_new,
              void (*destruct)(bufferevent *) = ::bufferevent_free>
    static Var<Bufferevent>
    openssl_filter_new(Var<EventBase> base, Var<Bufferevent> underlying,
                       ssl_st *ssl, enum bufferevent_ssl_state state,
                       int options) {
        Bufferevent *ptr = new Bufferevent();
        ptr->evbase = base;
        ptr->bevp =
            construct(base->evbase, underlying->bevp, ssl, state, options);
        if (ptr->bevp == nullptr) {
            delete ptr;
            MK_THROW(BuffereventOpensslFilterNewError);
        }
        underlying->bevp = nullptr; // Steal `bevp` from `underlying`
        // Clear eventual self-references that `underlying` could have...
        underlying->setcb(nullptr, nullptr, nullptr);
        // TODO: investigate the issue of SSL clean shutdown
        //  see <http://www.wangafu.net/~nickm/libevent-book/>
        //   and especially R6a, 'avanced bufferevents'
        Var<Bufferevent> *varp = new Var<Bufferevent>(ptr, [](Bufferevent *p) {
            if (p->bevp != nullptr) {
                destruct(p->bevp);
                p->bevp = nullptr;
            }
            delete p;
        });
#ifdef SRC_COMMON_LIBEVENT_CORE_ENABLE_MOCK
        ptr->the_opaque = varp;
#endif
        // We pass `varp` so C code keeps us alive
        ::bufferevent_setcb(ptr->bevp, mk_libevent_bev_read,
             mk_libevent_bev_write, mk_libevent_bev_event, varp);
        return *varp;
    }

    unsigned long get_openssl_error() {
        return ::bufferevent_get_openssl_error(bevp);
    }

    Var<Evbuffer> get_input() {
        return Evbuffer::assign(::bufferevent_get_input(bevp), false);
    }

    Var<Evbuffer> get_output() {
        return Evbuffer::assign(::bufferevent_get_output(bevp), false);
    }
};

} // namespace

#endif
