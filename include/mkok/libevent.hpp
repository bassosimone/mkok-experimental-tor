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
#include <openssl/ssl.h>
#include <string>
#include <utility>

// Forward declarations
struct bufferevent;
struct evbuffer;
struct event_base;
struct sockaddr;
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
    EVUTIL_MAKE_SOCKET_NONBLOCKING = 1000,
    EVUTIL_PARSE_SOCKADDR_PORT,
    EVUTIL_MAKE_LISTEN_SOCKET_REUSEABLE,
    EVENT_BASE_DISPATCH,
    EVENT_BASE_LOOP,
    EVENT_BASE_LOOPBREAK,
    EVENT_BASE_ONCE,
    EVBUFFER_ADD,
    EVBUFFER_ADD_BUFFER,
    EVBUFFER_PEEK,
    EVBUFFER_PEEK_MISMATCH,
    EVBUFFER_PULLUP,
    EVBUFFER_DRAIN,
    EVBUFFER_REMOVE_BUFFER,
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
XX(EvutilMakeSocketNonblockingError, EVUTIL_MAKE_SOCKET_NONBLOCKING);
XX(EvutilParseSockaddrPortError, EVUTIL_PARSE_SOCKADDR_PORT);
XX(EvutilMakeListenSocketReuseableError, EVUTIL_MAKE_LISTEN_SOCKET_REUSEABLE);
XX(EventBaseDispatchError, EVENT_BASE_DISPATCH);
XX(EventBaseLoopError, EVENT_BASE_LOOP);
XX(EventBaseLoopbreakError, EVENT_BASE_LOOPBREAK);
XX(EventBaseOnceError, EVENT_BASE_ONCE);
XX(EvbufferAddError, EVBUFFER_ADD);
XX(EvbufferAddBufferError, EVBUFFER_ADD_BUFFER);
XX(EvbufferDrainError, EVBUFFER_DRAIN);
XX(EvbufferPeekError, EVBUFFER_PEEK);
XX(EvbufferPeekMismatchError, EVBUFFER_PEEK_MISMATCH);
XX(EvbufferPullupError, EVBUFFER_PULLUP);
XX(EvbufferRemoveBufferError, EVBUFFER_REMOVE_BUFFER);
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

class Mock {
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
    XX(evbuffer_add, int(evbuffer *, const void *, size_t));
    XX(evbuffer_add_buffer, int(evbuffer *, evbuffer *));
    XX(evbuffer_drain, int(evbuffer *, size_t));
    XX(evbuffer_free, void(evbuffer *));
    XX(evbuffer_new, evbuffer *());
    XX(evbuffer_peek,
       int(evbuffer *, ssize_t, evbuffer_ptr *, evbuffer_iovec *, int));
    XX(evbuffer_pullup, unsigned char *(evbuffer *, ssize_t));
    XX(evbuffer_remove_buffer, int(evbuffer *, evbuffer *, size_t));
    XX(evbuffer_search_eol, evbuffer_ptr(evbuffer *, evbuffer_ptr *, size_t *,
                                         enum evbuffer_eol_style));
    XX(event_base_dispatch, int(event_base *));
    XX(event_base_free, void(event_base *));
    XX(event_base_loop, int(event_base *, int));
    XX(event_base_loopbreak, int(event_base *));
    XX(event_base_new, event_base *());
    XX(event_base_once, int(event_base *, evutil_socket_t, short,
                            event_callback_fn, void *, const timeval *));
    XX(evutil_make_listen_socket_reuseable, int(evutil_socket_t));
    XX(evutil_make_socket_nonblocking, int(evutil_socket_t));
    XX(evutil_parse_sockaddr_port, int(const char *, sockaddr *, int *));
    XX(SSL_free, void(ssl_st *));
#undef XX
};

#define MockPtrArg Mock *mockp,
#define MockPtrArg0 Mock *mockp
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

class Evutil {
  public:
    static void make_socket_nonblocking(MockPtrArg evutil_socket_t s) {
        if (call(evutil_make_socket_nonblocking, s) != 0) {
            MKOK_THROW(EvutilMakeSocketNonblockingError);
        }
    }

    static void parse_sockaddr_port(MockPtrArg std::string s, sockaddr *p,
                                    int *n) {
        if (call(evutil_parse_sockaddr_port, s.c_str(), p, n) != 0) {
            MKOK_THROW(EvutilParseSockaddrPortError);
        }
    }

    static void make_listen_socket_reuseable(MockPtrArg evutil_socket_t s) {
        if (call(evutil_make_listen_socket_reuseable, s) != 0) {
            MKOK_THROW(EvutilMakeListenSocketReuseableError);
        }
    }
};

class EventBase {
  public:
    event_base *evbase = nullptr;
    bool owned = false;
#ifdef MKOK_LIBEVENT_ENABLE_MOCK
    Mock *mockp = nullptr;
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

class Evbuffer {
  public:
    evbuffer *evbuf = nullptr;
    bool owned = false;
#ifdef MKOK_LIBEVENT_ENABLE_MOCK
    Mock *mockp = nullptr;
#endif

    Evbuffer() {}

    Evbuffer(Evbuffer &) = delete;
    Evbuffer &operator=(Evbuffer &) = delete;
    Evbuffer(Evbuffer &&) = delete;
    Evbuffer &operator=(Evbuffer &&) = delete;

    ~Evbuffer() {
        if (owned && evbuf != nullptr) {
            call(evbuffer_free, evbuf);
            owned = false;
            evbuf = nullptr;
        }
    }

    static Var<Evbuffer> assign(MockPtrArg evbuffer *pointer, bool owned) {
        if (pointer == nullptr) {
            MKOK_THROW(NullPointerError);
        }
        Var<Evbuffer> evbuf(new Evbuffer);
        evbuf->evbuf = pointer;
        evbuf->owned = owned;
#ifdef MKOK_LIBEVENT_ENABLE_MOCK
        evbuf->mockp = mockp;
#endif
        return evbuf;
    }

    static Var<Evbuffer> create(MockPtrArg0) {
        return assign(MockPtrName call(evbuffer_new), true);
    }

    static size_t get_length(Var<Evbuffer> evbuf) {
        return ::evbuffer_get_length(evbuf->evbuf);
    }

    static std::string pullup(MockPtrArg Var<Evbuffer> evbuf, ssize_t n) {
        unsigned char *s = call(evbuffer_pullup, evbuf->evbuf, n);
        if (s == nullptr) {
            MKOK_THROW(EvbufferPullupError);
        }
        return std::string((char *)s, get_length(evbuf));
    }

    static void drain(MockPtrArg Var<Evbuffer> evbuf, size_t n) {
        if (call(evbuffer_drain, evbuf->evbuf, n) != 0) {
            MKOK_THROW(EvbufferDrainError);
        }
    }

    static void add(MockPtrArg Var<Evbuffer> evbuf, const void *base,
                    size_t count) {
        if (call(evbuffer_add, evbuf->evbuf, base, count) != 0) {
            MKOK_THROW(EvbufferAddError);
        }
    }

    static void add_buffer(MockPtrArg Var<Evbuffer> evbuf, Var<Evbuffer> b) {
        if (call(evbuffer_add_buffer, evbuf->evbuf, b->evbuf) != 0) {
            MKOK_THROW(EvbufferAddBufferError);
        }
    }

    static Var<evbuffer_iovec> peek(MockPtrArg Var<Evbuffer> evbuf, ssize_t len,
                                    evbuffer_ptr *start_at, size_t &n_extents) {
        int required =
            call(evbuffer_peek, evbuf->evbuf, len, start_at, nullptr, 0);
        if (required < 0) {
            MKOK_THROW(EvbufferPeekError);
        }
        if (required == 0) {
            return nullptr; // Caller required to check return value
        }
        Var<evbuffer_iovec> retval(new evbuffer_iovec[required],
                                   [](evbuffer_iovec *p) { delete[] p; });
        evbuffer_iovec *iov = retval.get();
        int used =
            call(evbuffer_peek, evbuf->evbuf, len, start_at, iov, required);
        if (used != required) {
            MKOK_THROW(EvbufferPeekMismatchError);
        }
        // Cast to unsigned safe because we excluded negative case above
        n_extents = (unsigned)required;
        return retval;
    }

    static void for_each_(MockPtrArg Var<Evbuffer> evbuf,
                          std::function<bool(const void *, size_t)> cb) {
        // Not part of libevent API but useful to wrap part of such API
        size_t n_extents = 0;
        Var<evbuffer_iovec> raii =
            peek(MockPtrName evbuf, -1, nullptr, n_extents);
        if (!raii) return; // Prevent exception if pointer is null
        auto iov = raii.get();
        for (size_t i = 0; i < n_extents; ++i) {
            if (!cb(iov[i].iov_base, iov[i].iov_len)) {
                break;
            }
        }
    }

    static std::string copyout(MockPtrArg Var<Evbuffer> evbuf, size_t upto) {
        std::string out;
        for_each_(MockPtrName evbuf, [&out, &upto](const void *p, size_t n) {
            if (upto < n) n = upto;
            out.append((const char *)p, n);
            upto -= n;
            return (upto > 0);
        });
        return out;
    }

    static std::string remove(MockPtrArg Var<Evbuffer> evbuf, size_t upto) {
        std::string out = copyout(MockPtrName evbuf, upto);
        if (out.size() > 0) {
            drain(MockPtrName evbuf, out.size());
        }
        return out;
    }

    static int remove_buffer(MockPtrArg Var<Evbuffer> evbuf, Var<Evbuffer> b,
                             size_t count) {
        int len = call(evbuffer_remove_buffer, evbuf->evbuf, b->evbuf, count);
        if (len < 0) {
            MKOK_THROW(EvbufferRemoveBufferError);
        }
        return len;
    }

    static std::string readln(MockPtrArg Var<Evbuffer> evbuf,
                              enum evbuffer_eol_style style) {
        size_t eol_length = 0;
        auto sre = call(evbuffer_search_eol, evbuf->evbuf, nullptr, &eol_length,
                        style);
        if (sre.pos < 0) {
            return "";
        }
        // Note: pos is ssize_t and we have excluded the negative case above
        std::string out = remove(MockPtrName evbuf, (size_t)sre.pos);
        drain(MockPtrName evbuf, eol_length);
        return out;
    }
};

class Bufferevent {
  public:
    Func<void(short)> event_cb;
    Func<void()> read_cb;
    Func<void()> write_cb;
    bufferevent *bevp = nullptr;
    Var<Bufferevent> underlying;

    Bufferevent() {}
    Bufferevent(Bufferevent &) = delete;
    Bufferevent &operator=(Bufferevent &) = delete;
    Bufferevent(Bufferevent &&) = delete;
    Bufferevent &operator=(Bufferevent &&) = delete;

    ~Bufferevent() {
        // Note: deletion performed in smart-pointer's destructor
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
        ptr->bevp = call(bufferevent_socket_new, base->evbase, fd, flags);
        if (ptr->bevp == nullptr) {
            delete ptr;
            MKOK_THROW(BuffereventSocketNewError);
        }
        call(bufferevent_setcb, ptr->bevp, mkok_libevent_bev_read,
             mkok_libevent_bev_write, mkok_libevent_bev_event, ptr);
        // Note that the lambda closure keeps `base` alive
        return Var<Bufferevent>(ptr, [MockPtrName base](Bufferevent * p) {
            call(bufferevent_free, p->bevp);
            delete p;
        });
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
        ptr->underlying = underlying; // This keeps underlying alive
        ptr->bevp = call(bufferevent_openssl_filter_new, base->evbase,
                         underlying->bevp, ssl, state, 0);
        if (ptr->bevp == nullptr) {
            delete ptr;
            MKOK_THROW(BuffereventOpensslFilterNewError);
        }
        // Clear eventual self-references that `underlying` could have such that
        // when we clear `ptr->underlying` this should be enough to destroy it
        setcb(underlying, nullptr, nullptr, nullptr);
        call(bufferevent_setcb, ptr->bevp, mkok_libevent_bev_read,
             mkok_libevent_bev_write, mkok_libevent_bev_event, ptr);
        // Note that the lambda closure keeps `base` alive
        return Var<Bufferevent>(
            ptr, [ MockPtrName base, options, ssl ](Bufferevent * p) {
                // We must override the implementation of BEV_OPT_CLOSE_ON_FREE
                // since that flag recursively destructs the underlying bufev
                // whose lifecycle however is now bound to the controlling ptr
                if ((options & BEV_OPT_CLOSE_ON_FREE) != 0) {
                    // TODO: investigate the issue of SSL clean shutdown
                    //  see <http://www.wangafu.net/~nickm/libevent-book/>
                    //   and especially R6a, 'avanced bufferevents'
                    call(SSL_free, ssl);
                }
                call(bufferevent_free, p->bevp);
                p->underlying = nullptr; // This should dispose of underlying
                delete p;
            });
    }

    static unsigned long get_openssl_error(Var<Bufferevent> bev) {
        return ::bufferevent_get_openssl_error(bev->bevp);
    }

    static Var<Evbuffer> get_input(MockPtrArg Var<Bufferevent> bev) {
        return Evbuffer::assign(MockPtrName bufferevent_get_input(bev->bevp),
                                false);
    }

    static Var<Evbuffer> get_output(MockPtrArg Var<Bufferevent> bev) {
        return Evbuffer::assign(MockPtrName bufferevent_get_output(bev->bevp),
                                false);
    }
};

// Undefine internal macros
#undef MockPtrArg
#undef MockPtrArg0
#undef MockPtrName
#undef call

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

static void mkok_libevent_bev_read(bufferevent *, void *ptr) {
    auto ctx = static_cast<XX Bufferevent *>(ptr);
    if (!ctx->read_cb) return;
    ctx->read_cb();
}

static void mkok_libevent_bev_write(bufferevent *, void *ptr) {
    auto ctx = static_cast<XX Bufferevent *>(ptr);
    if (!ctx->write_cb) return;
    ctx->write_cb();
}

static void mkok_libevent_bev_event(bufferevent *, short what, void *ptr) {
    auto ctx = static_cast<XX Bufferevent *>(ptr);
    if (!ctx->event_cb) return;
    ctx->event_cb(what);
}

#undef XX

} // extern "C"

#endif
