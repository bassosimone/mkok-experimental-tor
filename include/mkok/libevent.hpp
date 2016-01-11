// Part of measurement-kit <https://measurement-kit.github.io/>.
// Measurement-kit is free software. See AUTHORS and LICENSE for more
// information on the copying conditions.
#ifndef MKOK_LIBEVENT_HPP
#define MKOK_LIBEVENT_HPP

#include <cstddef>
#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <event2/bufferevent_ssl.h>
#include <event2/event.h>
#include <event2/util.h>
#include <exception>
#include <functional>
#include <memory>
#include <mkok/base.hpp>
#include <mkok/evbuffer.hpp>
#include <string>
#include <utility>

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

static void mkok_libevent_bev_read(bufferevent *, void *ptr);
static void mkok_libevent_bev_write(bufferevent *, void *ptr);
static void mkok_libevent_bev_event(bufferevent *, short what, void *ptr);
static void mkok_libevent_event_cb(evutil_socket_t, short w, void *p);

} // extern "C"

#ifdef MKOK_NAMESPACE
namespace MKOK_NAMESPACE {
#endif

/// \addtogroup errors
/// \{

// Note that libevent error codes MUST start from 1000
enum LibeventErrorCode {
    EVENT_BASE_DISPATCH,
    EVENT_BASE_LOOP,
    EVENT_BASE_LOOPBREAK,
    EVENT_BASE_ONCE,
    BUFFEREVENT_SOCKET_NEW,
    BUFFEREVENT_SOCKET_CONNECT,
    BUFFEREVENT_WRITE,
    BUFFEREVENT_WRITE_BUFFER,
    BUFFEREVENT_READ_BUFFER,
    BUFFEREVENT_ENABLE,
    BUFFEREVENT_DISABLE,
    BUFFEREVENT_SET_TIMEOUTS,
    BUFFEREVENT_OPENSSL_FILTER_NEW,
};

#define XX(a, b) MKOK_DEFINE_ERROR(a, LibeventErrorCode::b)
XX(EventBaseDispatchError, EVENT_BASE_DISPATCH);
XX(EventBaseLoopError, EVENT_BASE_LOOP);
XX(EventBaseLoopbreakError, EVENT_BASE_LOOPBREAK);
XX(EventBaseOnceError, EVENT_BASE_ONCE);
XX(BuffereventSocketNewError, BUFFEREVENT_SOCKET_NEW);
XX(BuffereventSocketConnectError, BUFFEREVENT_SOCKET_CONNECT);
XX(BuffereventWriteError, BUFFEREVENT_WRITE);
XX(BuffereventWriteBufferError, BUFFEREVENT_WRITE_BUFFER);
XX(BuffereventReadBufferError, BUFFEREVENT_READ_BUFFER);
XX(BuffereventEnableError, BUFFEREVENT_ENABLE);
XX(BuffereventDisableError, BUFFEREVENT_DISABLE);
XX(BuffereventSetTimeoutsError, BUFFEREVENT_SET_TIMEOUTS);
XX(BuffereventOpensslFilterNewError, BUFFEREVENT_OPENSSL_FILTER_NEW);
#undef XX

/// \}
/// \addtogroup regress
/// \{

#ifdef MKOK_LIBEVENT_ENABLE_MOCK

class LibeventMock {
  public:
#define XX(name, signature) std::function<signature> name = ::name
    XX(bufferevent_disable, int(bufferevent *, int));
    XX(bufferevent_enable, int(bufferevent *, int));
    XX(bufferevent_free, void(bufferevent *));
    XX(bufferevent_openssl_filter_new,
       bufferevent *(event_base *, bufferevent *, ssl_st *,
                     bufferevent_ssl_state, int));
    XX(bufferevent_read_buffer, int(bufferevent *, evbuffer *));
    XX(bufferevent_setcb,
       void(bufferevent *, bufferevent_data_cb, bufferevent_data_cb,
            bufferevent_event_cb, void *));
    XX(bufferevent_set_timeouts,
       int(bufferevent *, const timeval *, const timeval *));
    XX(bufferevent_socket_connect, int(bufferevent *, sockaddr *, int));
    XX(bufferevent_socket_new,
       bufferevent *(event_base *, evutil_socket_t, int));
    XX(bufferevent_write, int(bufferevent *, const void *, size_t));
    XX(bufferevent_write_buffer, int(bufferevent *, evbuffer *));
    XX(event_base_dispatch, int(event_base *));
    XX(event_base_free, void(event_base *));
    XX(event_base_loop, int(event_base *, int));
    XX(event_base_loopbreak, int(event_base *));
    XX(event_base_new, event_base *());
    XX(event_base_once, int(event_base *, evutil_socket_t, short,
                            event_callback_fn, void *, const timeval *));
#undef XX
};

#define MockPtrArg LibeventMock *mockp,
#define MockPtrArg0 LibeventMock *mockp
#define MockPtrName mockp,
#define call(func, ...) mockp->func(__VA_ARGS__)

#else

#define MockPtrArg
#define MockPtrArg0
#define MockPtrName
#define call(func, ...) ::func(__VA_ARGS__)

#endif

/// \}
/// \addtogroup wrappers
/// \{

class EventBase {
  public:
    event_base *evbase = nullptr;
    bool owned = false;
#ifdef MKOK_LIBEVENT_ENABLE_MOCK
    LibeventMock *mockp = nullptr;
#endif

    EventBase() {}
    EventBase(EventBase &) = delete;
    EventBase &operator=(EventBase &) = delete;
    EventBase(EventBase &&) = delete;
    EventBase &operator=(EventBase &&) = delete;

    ~EventBase() {
        if (owned && evbase != nullptr) {
            call(event_base_free, evbase);
            owned = false;
            evbase = nullptr;
        }
    }

    static Var<EventBase> assign(MockPtrArg event_base *pointer, bool owned) {
        if (pointer == nullptr) {
            MKOK_THROW(NullPointerError);
        }
        Var<EventBase> base(new EventBase);
        base->evbase = pointer;
        base->owned = owned;
#ifdef MKOK_LIBEVENT_ENABLE_MOCK
        base->mockp = mockp;
#endif
        return base;
    }

    static Var<EventBase> create(MockPtrArg0) {
        return assign(MockPtrName call(event_base_new), true);
    }

    static int dispatch(MockPtrArg Var<EventBase> base) {
        int ctrl = call(event_base_dispatch, base->evbase);
        if (ctrl != 0 && ctrl != 1) {
            MKOK_THROW(EventBaseDispatchError);
        }
        return ctrl;
    }

    static int loop(MockPtrArg Var<EventBase> base, int flags) {
        int ctrl = call(event_base_loop, base->evbase, flags);
        if (ctrl != 0 && ctrl != 1) {
            MKOK_THROW(EventBaseLoopError);
        }
        return ctrl;
    }

    static void loopbreak(MockPtrArg Var<EventBase> base) {
        if (call(event_base_loopbreak, base->evbase) != 0) {
            MKOK_THROW(EventBaseLoopbreakError);
        }
    }

    static void once(MockPtrArg Var<EventBase> base, evutil_socket_t sock,
                     short what, std::function<void(short)> callback,
                     const timeval *timeo = nullptr) {
        auto func = new std::function<void(short)>(callback);
        if (call(event_base_once, base->evbase, sock, what,
                 mkok_libevent_event_cb, func, timeo) != 0) {
            delete func;
            MKOK_THROW(EventBaseOnceError);
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
#ifdef MKOK_LIBEVENT_ENABLE_MOCK
    LibeventMock *mockp = nullptr;
#endif

    Bufferevent() {}
    Bufferevent(Bufferevent &) = delete;
    Bufferevent &operator=(Bufferevent &) = delete;
    Bufferevent(Bufferevent &&) = delete;
    Bufferevent &operator=(Bufferevent &&) = delete;

    ~Bufferevent() {
        if (bevp != nullptr) {
            call(bufferevent_free, bevp);
            bevp = nullptr;
        }
    }

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

    static Var<Bufferevent> socket_new(MockPtrArg Var<EventBase> base,
                                       evutil_socket_t fd, int flags) {
        Bufferevent *ptr = new Bufferevent;
        ptr->evbase = base;
#ifdef MKOK_LIBEVENT_ENABLE_MOCK
        ptr->mockp = mockp;
#endif
        ptr->bevp = call(bufferevent_socket_new, base->evbase, fd, flags);
        if (ptr->bevp == nullptr) {
            delete ptr;
            MKOK_THROW(BuffereventSocketNewError);
        }
        Var<Bufferevent> *varp = new Var<Bufferevent>(ptr);
        // We pass `varp` so C code keeps us alive
        call(bufferevent_setcb, ptr->bevp, mkok_libevent_bev_read,
             mkok_libevent_bev_write, mkok_libevent_bev_event, varp);
        return *varp;
    }

    static void socket_connect(MockPtrArg Var<Bufferevent> bev, sockaddr *sa,
                               int len) {
        if (call(bufferevent_socket_connect, bev->bevp, sa, len) != 0) {
            MKOK_THROW(BuffereventSocketConnectError);
        }
    }

    static void setcb(Var<Bufferevent> bev, std::function<void()> readcb,
                      std::function<void()> writecb,
                      std::function<void(short)> eventcb) {
        bev->event_cb = eventcb;
        bev->read_cb = readcb;
        bev->write_cb = writecb;
    }

    static void write(MockPtrArg Var<Bufferevent> bev, const void *base,
                      size_t count) {
        if (call(bufferevent_write, bev->bevp, base, count) != 0) {
            MKOK_THROW(BuffereventWriteError);
        }
    }

    static void write_buffer(MockPtrArg Var<Bufferevent> bev, Var<Evbuffer> s) {
        if (call(bufferevent_write_buffer, bev->bevp, s->evbuf) != 0) {
            MKOK_THROW(BuffereventWriteBufferError);
        }
    }

    static size_t read(Var<Bufferevent> bev, void *base, size_t count) {
        return ::bufferevent_read(bev->bevp, base, count);
    }

    static void read_buffer(MockPtrArg Var<Bufferevent> bev, Var<Evbuffer> d) {
        if (call(bufferevent_read_buffer, bev->bevp, d->evbuf) != 0) {
            MKOK_THROW(BuffereventReadBufferError);
        }
    }

    static void enable(MockPtrArg Var<Bufferevent> bev, short what) {
        if (call(bufferevent_enable, bev->bevp, what) != 0) {
            MKOK_THROW(BuffereventEnableError);
        }
    }

    static void disable(MockPtrArg Var<Bufferevent> bev, short what) {
        if (call(bufferevent_disable, bev->bevp, what) != 0) {
            MKOK_THROW(BuffereventDisableError);
        }
    }

    static void set_timeouts(MockPtrArg Var<Bufferevent> bev,
                             const timeval *rto, const timeval *wto) {
        if (call(bufferevent_set_timeouts, bev->bevp, rto, wto) != 0) {
            MKOK_THROW(BuffereventSetTimeoutsError);
        }
    }

    static Var<Bufferevent> openssl_filter_new(MockPtrArg Var<EventBase> base,
                                               Var<Bufferevent> underlying,
                                               ssl_st *ssl,
                                               enum bufferevent_ssl_state state,
                                               int options) {
        Bufferevent *ptr = new Bufferevent();
        ptr->evbase = base;
#ifdef MKOK_LIBEVENT_ENABLE_MOCK
        ptr->mockp = mockp;
#endif
        ptr->bevp = call(bufferevent_openssl_filter_new, base->evbase,
                         underlying->bevp, ssl, state, options);
        if (ptr->bevp == nullptr) {
            delete ptr;
            MKOK_THROW(BuffereventOpensslFilterNewError);
        }
        underlying->bevp = nullptr; // Steal `bevp` from `underlying`
        // Clear eventual self-references that `underlying` could have such that
        // when we clear `ptr->underlying` this should be enough to destroy it
        setcb(underlying, nullptr, nullptr, nullptr);
        // TODO: investigate the issue of SSL clean shutdown
        //  see <http://www.wangafu.net/~nickm/libevent-book/>
        //   and especially R6a, 'avanced bufferevents'
        Var<Bufferevent> *varp = new Var<Bufferevent>(ptr);
        // We pass `varp` so C code keeps us alive
        call(bufferevent_setcb, ptr->bevp, mkok_libevent_bev_read,
             mkok_libevent_bev_write, mkok_libevent_bev_event, varp);
        return *varp;
    }

    static unsigned long get_openssl_error(Var<Bufferevent> bev) {
        return ::bufferevent_get_openssl_error(bev->bevp);
    }

    static Var<Evbuffer> get_input(Var<Bufferevent> bev) {
        return Evbuffer::assign(bufferevent_get_input(bev->bevp), false);
    }

    static Var<Evbuffer> get_output(Var<Bufferevent> bev) {
        return Evbuffer::assign(bufferevent_get_output(bev->bevp), false);
    }
};

// Undefine internal macros
#undef MockPtrArg
#undef MockPtrArg0

/// \}

#ifdef MKOK_NAMESPACE
} // namespace
#endif

extern "C" {

static void mkok_libevent_event_cb(evutil_socket_t, short w, void *p) {
    auto f = static_cast<std::function<void(short)> *>(p);
    (*f)(w);
    delete f;
}

#ifdef MKOK_NAMESPACE
#define XX MKOK_NAMESPACE::
#else
#define XX
#endif

#ifdef MKOK_LIBEVENT_ENABLE_MOCK
#undef call
#define call(func, ...) (*varp)->mockp->func(__VA_ARGS__)
#undef MockPtrName
#define MockPtrName (*varp)->mockp,
#endif

static void mkok_libevent_bev_is_ignored_by_cxx(Var<Bufferevent> *varp);

static void mkok_libevent_bev_read(bufferevent *, void *ptr) {
    XX Var<XX Bufferevent> *varp = static_cast<XX Var<XX Bufferevent> *>(ptr);
    if ((*varp)->read_cb) (*varp)->read_cb();
    if (varp->unique()) mkok_libevent_bev_is_ignored_by_cxx(varp);
}

static void mkok_libevent_bev_write(bufferevent *, void *ptr) {
    XX Var<XX Bufferevent> *varp = static_cast<XX Var<XX Bufferevent> *>(ptr);
    if ((*varp)->write_cb) (*varp)->write_cb();
    if (varp->unique()) mkok_libevent_bev_is_ignored_by_cxx(varp);
}

static void mkok_libevent_bev_event(bufferevent *, short what, void *ptr) {
    XX Var<XX Bufferevent> *varp = static_cast<XX Var<XX Bufferevent> *>(ptr);
    if ((*varp)->event_cb) (*varp)->event_cb(what);
    if (varp->unique()) mkok_libevent_bev_is_ignored_by_cxx(varp);
}

static void mkok_libevent_bev_is_ignored_by_cxx(XX Var<XX Bufferevent> *varp) {
    // If a smart pointer is ignored by C++ code, we can dispose of it.
    // If `bevp` is not null, we put it into a state in which it cannot hurt,
    // otherwise, if it's null, it was stolen by SSL code.
    // We do not delete the pointer immediately because the underlying code
    // that called us may not like it; instead postpone to next cycle.
    // Calling delete is going to trigger bufferevent_free().
    if ((*varp)->bevp != nullptr) {
        call(bufferevent_setcb, (*varp)->bevp, nullptr, nullptr, nullptr,
             nullptr);
        if (call(bufferevent_disable, (*varp)->bevp, EV_READ|EV_WRITE) != 0) {
            MKOK_THROW(BuffereventDisableError);
        }
    }
    timeval timeo;
    timeo.tv_sec = timeo.tv_usec = 0;
    EventBase::once(MockPtrName (*varp)->evbase, -1, EV_TIMEOUT, [varp](short) {
        delete varp;
    }, &timeo);
}

// Undef more internals
#undef MockPtrName
#undef XX
#undef call

} // extern "C"

#endif
