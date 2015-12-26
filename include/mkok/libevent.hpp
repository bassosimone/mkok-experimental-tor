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

#ifdef MKOK_LIBEVENT_NAMESPACE
namespace MKOK_LIBEVENT_NAMESPACE {
#endif

/// \addtogroup errors
/// @{

#define MKOK_LIBEVENT_NO_ERROR 0
#define MKOK_LIBEVENT_GENERIC_ERROR 1
#define MKOK_LIBEVENT_NULL_POINTER_ERROR 2
#define MKOK_LIBEVENT_LIBEVENT_ERROR 3

typedef int Error;

class Exception : public std::exception {
  public:
    std::string file;
    int line = 0;
    std::string func;
    Error error = MKOK_LIBEVENT_GENERIC_ERROR;

    Exception() {}

    Exception(const char *f, int n, const char *fn, Error c)
        : file(f), line(n), func(fn), error(c) {}

    ~Exception() {}
};

#define XX(class_name, error_code)                                             \
    class class_name : public Exception {                                      \
      public:                                                                  \
        class_name(const char *f, int n, const char *fn)                       \
            : Exception(f, n, fn, error_code) {}                               \
    }
XX(NoException, MKOK_LIBEVENT_NO_ERROR);
XX(GenericException, MKOK_LIBEVENT_GENERIC_ERROR);
XX(NullPointerException, MKOK_LIBEVENT_NULL_POINTER_ERROR);
XX(LibeventException, MKOK_LIBEVENT_LIBEVENT_ERROR);
#undef XX

#define MKOK_LIBEVENT_THROW(class_name)                                        \
    throw class_name(__FILE__, __LINE__, __func__)

/// @}
/// \addtogroup util
/// @{

template <typename T> class Var : public std::shared_ptr<T> {
    // BEWARE NOT TO ADD ANY ATTRIBUTE TO THIS CLASS BECAUSE THAT MAY
    // LEAD TO OBJECT SLICING AND SUBTLE BUGS

  public:
    using std::shared_ptr<T>::shared_ptr;

    T *get() const {
        // Provides no-null-pointer-returned guarantee
        T *p = std::shared_ptr<T>::get();
        if (p == nullptr) {
            MKOK_LIBEVENT_THROW(NullPointerException);
        }
        return p;
    }

    T *operator->() const { return get(); }
};

template <typename T> class Func {
  public:
    Func() {}
    Func(std::function<T> f) : func(f) {}

    ~Func() {}

    void operator=(std::function<T> f) { func = f; }
    void operator=(std::nullptr_t f) { func = f; }

    // not implementing swap and assign

    operator bool() { return bool(func); }

    template <typename... Args> void operator()(Args &&... args) {
        // Make sure the original closure is not destroyed before end of scope
        auto backup = func;
        backup(std::forward<Args>(args)...);
    }

  private:
    std::function<T> func;
};

/// @}
/// \addtogroup regress
/// @{

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
    XX(bufferevent_read, size_t(bufferevent *, void *, size_t));
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
    XX(evbuffer_free, void(evbuffer *));
    XX(evbuffer_new, evbuffer *());
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

#define MKOK_LIBEVENT_MOCKP Mock *mockp,
#define MKOK_LIBEVENT_MOCKP0 Mock *mockp
#define MKOK_LIBEVENT_MOCKP_NAME mockp,

#define MKOK_LIBEVENT_BUFFEREVENT_DISABLE mockp->bufferevent_disable
#define MKOK_LIBEVENT_BUFFEREVENT_ENABLE mockp->bufferevent_enable
#define MKOK_LIBEVENT_BUFFEREVENT_FREE mockp->bufferevent_free
#define MKOK_LIBEVENT_BUFFEREVENT_OPENSSL_FILTER_NEW                           \
    mockp->bufferevent_openssl_filter_new
#define MKOK_LIBEVENT_BUFFEREVENT_READ mockp->bufferevent_read
#define MKOK_LIBEVENT_BUFFEREVENT_READ_BUFFER mockp->bufferevent_read_buffer
#define MKOK_LIBEVENT_BUFFEREVENT_SETCB mockp->bufferevent_setcb
#define MKOK_LIBEVENT_BUFFEREVENT_SET_TIMEOUTS mockp->bufferevent_set_timeouts
#define MKOK_LIBEVENT_BUFFEREVENT_SOCKET_CONNECT                               \
    mockp->bufferevent_socket_connect
#define MKOK_LIBEVENT_BUFFEREVENT_SOCKET_NEW mockp->bufferevent_socket_new
#define MKOK_LIBEVENT_BUFFEREVENT_WRITE mockp->bufferevent_write
#define MKOK_LIBEVENT_BUFFEREVENT_WRITE_BUFFER mockp->bufferevent_write_buffer
#define MKOK_LIBEVENT_EVBUFFER_FREE mockp->evbuffer_free
#define MKOK_LIBEVENT_EVBUFFER_NEW mockp->evbuffer_new
#define MKOK_LIBEVENT_EVENT_BASE_DISPATCH mockp->event_base_dispatch
#define MKOK_LIBEVENT_EVENT_BASE_FREE mockp->event_base_free
#define MKOK_LIBEVENT_EVENT_BASE_LOOP mockp->event_base_loop
#define MKOK_LIBEVENT_EVENT_BASE_LOOPBREAK mockp->event_base_loopbreak
#define MKOK_LIBEVENT_EVENT_BASE_NEW mockp->event_base_new
#define MKOK_LIBEVENT_EVENT_BASE_ONCE mockp->event_base_once
#define MKOK_LIBEVENT_EVUTIL_MAKE_LISTEN_SOCKET_REUSEABLE                      \
    mockp->evutil_make_listen_socket_reuseable
#define MKOK_LIBEVENT_EVUTIL_MAKE_SOCKET_NONBLOCKING                           \
    mockp->evutil_make_socket_nonblocking
#define MKOK_LIBEVENT_EVUTIL_PARSE_SOCKADDR_PORT                               \
    mockp->evutil_parse_sockaddr_port
#define MKOK_LIBEVENT_SSL_FREE mockp->SSL_free

#else

#define MKOK_LIBEVENT_MOCKP
#define MKOK_LIBEVENT_MOCKP0
#define MKOK_LIBEVENT_MOCKP_NAME

#define MKOK_LIBEVENT_BUFFEREVENT_DISABLE ::bufferevent_disable
#define MKOK_LIBEVENT_BUFFEREVENT_ENABLE ::bufferevent_enable
#define MKOK_LIBEVENT_BUFFEREVENT_FREE ::bufferevent_free
#define MKOK_LIBEVENT_BUFFEREVENT_OPENSSL_FILTER_NEW                           \
    ::bufferevent_openssl_filter_new
#define MKOK_LIBEVENT_BUFFEREVENT_READ ::bufferevent_read
#define MKOK_LIBEVENT_BUFFEREVENT_READ_BUFFER ::bufferevent_read_buffer
#define MKOK_LIBEVENT_BUFFEREVENT_SETCB ::bufferevent_setcb
#define MKOK_LIBEVENT_BUFFEREVENT_SET_TIMEOUTS ::bufferevent_set_timeouts
#define MKOK_LIBEVENT_BUFFEREVENT_SOCKET_CONNECT ::bufferevent_socket_connect
#define MKOK_LIBEVENT_BUFFEREVENT_SOCKET_NEW ::bufferevent_socket_new
#define MKOK_LIBEVENT_BUFFEREVENT_WRITE ::bufferevent_write
#define MKOK_LIBEVENT_BUFFEREVENT_WRITE_BUFFER ::bufferevent_write_buffer
#define MKOK_LIBEVENT_EVBUFFER_FREE ::evbuffer_free
#define MKOK_LIBEVENT_EVBUFFER_NEW ::evbuffer_new
#define MKOK_LIBEVENT_EVENT_BASE_DISPATCH ::event_base_dispatch
#define MKOK_LIBEVENT_EVENT_BASE_FREE ::event_base_free
#define MKOK_LIBEVENT_EVENT_BASE_LOOP ::event_base_loop
#define MKOK_LIBEVENT_EVENT_BASE_LOOPBREAK ::event_base_loopbreak
#define MKOK_LIBEVENT_EVENT_BASE_NEW ::event_base_new
#define MKOK_LIBEVENT_EVENT_BASE_ONCE ::event_base_once
#define MKOK_LIBEVENT_EVUTIL_MAKE_LISTEN_SOCKET_REUSEABLE                      \
    ::evutil_make_listen_socket_reuseable
#define MKOK_LIBEVENT_EVUTIL_MAKE_SOCKET_NONBLOCKING                           \
    ::evutil_make_socket_nonblocking
#define MKOK_LIBEVENT_EVUTIL_PARSE_SOCKADDR_PORT ::evutil_parse_sockaddr_port
#define MKOK_LIBEVENT_SSL_FREE ::SSL_free

#endif

/// @}
/// \addtogroup wrappers
/// @{

class Evutil {
  public:
    static void make_socket_nonblocking(MKOK_LIBEVENT_MOCKP evutil_socket_t s) {
        if (MKOK_LIBEVENT_EVUTIL_MAKE_SOCKET_NONBLOCKING(s) != 0) {
            MKOK_LIBEVENT_THROW(LibeventException);
        }
    }

    static void parse_sockaddr_port(MKOK_LIBEVENT_MOCKP std::string s,
                                    sockaddr *p, int *n) {
        if (MKOK_LIBEVENT_EVUTIL_PARSE_SOCKADDR_PORT(s.c_str(), p, n) != 0) {
            MKOK_LIBEVENT_THROW(LibeventException);
        }
    }

    static void
    make_listen_socket_reuseable(MKOK_LIBEVENT_MOCKP evutil_socket_t s) {
        if (MKOK_LIBEVENT_EVUTIL_MAKE_LISTEN_SOCKET_REUSEABLE(s) != 0) {
            MKOK_LIBEVENT_THROW(LibeventException);
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
            MKOK_LIBEVENT_EVENT_BASE_FREE(evbase);
            owned = false;
            evbase = nullptr;
        }
    }

    static Var<EventBase> assign(MKOK_LIBEVENT_MOCKP event_base *pointer,
                                 bool owned) {
        if (pointer == nullptr) {
            MKOK_LIBEVENT_THROW(NullPointerException);
        }
        Var<EventBase> base(new EventBase);
        base->evbase = pointer;
        base->owned = owned;
#ifdef MKOK_LIBEVENT_ENABLE_MOCK
        base->mockp = mockp;
#endif
        return base;
    }

    static Var<EventBase> create(MKOK_LIBEVENT_MOCKP0) {
        return assign(MKOK_LIBEVENT_MOCKP_NAME MKOK_LIBEVENT_EVENT_BASE_NEW(),
                      true);
    }

    static int dispatch(MKOK_LIBEVENT_MOCKP Var<EventBase> base) {
        int ctrl = MKOK_LIBEVENT_EVENT_BASE_DISPATCH(base->evbase);
        if (ctrl != 0 && ctrl != 1) {
            MKOK_LIBEVENT_THROW(LibeventException);
        }
        return ctrl;
    }

    static int loop(MKOK_LIBEVENT_MOCKP Var<EventBase> base, int flags) {
        int ctrl = MKOK_LIBEVENT_EVENT_BASE_LOOP(base->evbase, flags);
        if (ctrl != 0 && ctrl != 1) {
            MKOK_LIBEVENT_THROW(LibeventException);
        }
        return ctrl;
    }

    static void loopbreak(MKOK_LIBEVENT_MOCKP Var<EventBase> base) {
        if (MKOK_LIBEVENT_EVENT_BASE_LOOPBREAK(base->evbase) != 0) {
            MKOK_LIBEVENT_THROW(LibeventException);
        }
    }

    static void once(MKOK_LIBEVENT_MOCKP Var<EventBase> base,
                     evutil_socket_t sock, short what,
                     std::function<void(short)> callback,
                     const timeval *timeo = nullptr) {
        auto func = new std::function<void(short)>(callback);
        if (MKOK_LIBEVENT_EVENT_BASE_ONCE(base->evbase, sock, what,
                                          mkok_libevent_event_cb, func,
                                          timeo) != 0) {
            MKOK_LIBEVENT_THROW(LibeventException);
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
            MKOK_LIBEVENT_EVBUFFER_FREE(evbuf);
            owned = false;
            evbuf = nullptr;
        }
    }

    static Var<Evbuffer> assign(MKOK_LIBEVENT_MOCKP evbuffer *pointer,
                                bool owned) {
        if (pointer == nullptr) {
            MKOK_LIBEVENT_THROW(NullPointerException);
        }
        Var<Evbuffer> evbuf(new Evbuffer);
        evbuf->evbuf = pointer;
        evbuf->owned = owned;
#ifdef MKOK_LIBEVENT_ENABLE_MOCK
        evbuf->mockp = mockp;
#endif
        return evbuf;
    }

    static Var<Evbuffer> create(MKOK_LIBEVENT_MOCKP0) {
        return assign(MKOK_LIBEVENT_MOCKP_NAME MKOK_LIBEVENT_EVBUFFER_NEW(),
                      true);
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

    static Var<Bufferevent> socket_new(MKOK_LIBEVENT_MOCKP Var<EventBase> base,
                                       evutil_socket_t fd, int flags) {
        Bufferevent *ptr = new Bufferevent;
        ptr->bevp =
            MKOK_LIBEVENT_BUFFEREVENT_SOCKET_NEW(base->evbase, fd, flags);
        if (ptr->bevp == nullptr) {
            delete ptr;
            MKOK_LIBEVENT_THROW(LibeventException);
        }
        MKOK_LIBEVENT_BUFFEREVENT_SETCB(ptr->bevp, mkok_libevent_bev_read,
                                        mkok_libevent_bev_write,
                                        mkok_libevent_bev_event, ptr);
        // Note that the lambda closure keeps `base` alive
        return Var<Bufferevent>(
            ptr, [MKOK_LIBEVENT_MOCKP_NAME base](Bufferevent * p) {
                MKOK_LIBEVENT_BUFFEREVENT_FREE(p->bevp);
                delete p;
            });
    }

    static void socket_connect(MKOK_LIBEVENT_MOCKP Var<Bufferevent> bev,
                               sockaddr *sa, int len) {
        if (MKOK_LIBEVENT_BUFFEREVENT_SOCKET_CONNECT(bev->bevp, sa, len) != 0) {
            MKOK_LIBEVENT_THROW(LibeventException);
        }
    }

    static void setcb(Var<Bufferevent> bev, std::function<void()> readcb,
                      std::function<void()> writecb,
                      std::function<void(short)> eventcb) {
        bev->event_cb = eventcb;
        bev->read_cb = readcb;
        bev->write_cb = writecb;
    }

    static void write(MKOK_LIBEVENT_MOCKP Var<Bufferevent> bev,
                      const void *base, size_t count) {
        if (MKOK_LIBEVENT_BUFFEREVENT_WRITE(bev->bevp, base, count) != 0) {
            MKOK_LIBEVENT_THROW(LibeventException);
        }
    }

    static void write_buffer(MKOK_LIBEVENT_MOCKP Var<Bufferevent> bev,
                             Var<Evbuffer> s) {
        if (MKOK_LIBEVENT_BUFFEREVENT_WRITE_BUFFER(bev->bevp, s->evbuf) != 0) {
            MKOK_LIBEVENT_THROW(LibeventException);
        }
    }

    static size_t read(MKOK_LIBEVENT_MOCKP Var<Bufferevent> bev, void *base,
                       size_t count) {
        return MKOK_LIBEVENT_BUFFEREVENT_READ(bev->bevp, base, count);
    }

    static void read_buffer(MKOK_LIBEVENT_MOCKP Var<Bufferevent> bev,
                            Var<Evbuffer> d) {
        if (MKOK_LIBEVENT_BUFFEREVENT_READ_BUFFER(bev->bevp, d->evbuf) != 0) {
            MKOK_LIBEVENT_THROW(LibeventException);
        }
    }

    static void enable(MKOK_LIBEVENT_MOCKP Var<Bufferevent> bev, short what) {
        if (MKOK_LIBEVENT_BUFFEREVENT_ENABLE(bev->bevp, what) != 0) {
            MKOK_LIBEVENT_THROW(LibeventException);
        }
    }

    static void disable(MKOK_LIBEVENT_MOCKP Var<Bufferevent> bev, short what) {
        if (MKOK_LIBEVENT_BUFFEREVENT_DISABLE(bev->bevp, what) != 0) {
            MKOK_LIBEVENT_THROW(LibeventException);
        }
    }

    static void set_timeouts(MKOK_LIBEVENT_MOCKP Var<Bufferevent> bev,
                             const timeval *rto, const timeval *wto) {
        if (MKOK_LIBEVENT_BUFFEREVENT_SET_TIMEOUTS(bev->bevp, rto, wto) != 0) {
            MKOK_LIBEVENT_THROW(LibeventException);
        }
    }

    static Var<Bufferevent>
    openssl_filter_new(MKOK_LIBEVENT_MOCKP Var<EventBase> base,
                       Var<Bufferevent> underlying, ssl_st *ssl,
                       enum bufferevent_ssl_state state, int options) {
        Bufferevent *ptr = new Bufferevent();
        ptr->underlying = underlying; // This keeps underlying alive
        ptr->bevp = MKOK_LIBEVENT_BUFFEREVENT_OPENSSL_FILTER_NEW(
            base->evbase, underlying->bevp, ssl, state, 0);
        if (ptr->bevp == nullptr) {
            delete ptr;
            MKOK_LIBEVENT_THROW(LibeventException);
        }
        // Clear eventual self-references that `underlying` could have such that
        // when we clear `ptr->underlying` this should be enough to destroy it
        setcb(underlying, nullptr, nullptr, nullptr);
        MKOK_LIBEVENT_BUFFEREVENT_SETCB(ptr->bevp, mkok_libevent_bev_read,
                                        mkok_libevent_bev_write,
                                        mkok_libevent_bev_event, ptr);
        // Note that the lambda closure keeps `base` alive
        return Var<Bufferevent>(
            ptr,
            [ MKOK_LIBEVENT_MOCKP_NAME base, options, ssl ](Bufferevent * p) {
                // We must override the implementation of BEV_OPT_CLOSE_ON_FREE
                // since that flag recursively destructs the underlying bufev
                // whose lifecycle however is now bound to the controlling ptr
                if ((options & BEV_OPT_CLOSE_ON_FREE) != 0) {
                    // TODO: investigate the issue of SSL clean shutdown
                    //  see <http://www.wangafu.net/~nickm/libevent-book/>
                    //   and especially R6a, 'avanced bufferevents'
                    MKOK_LIBEVENT_SSL_FREE(ssl);
                }
                MKOK_LIBEVENT_BUFFEREVENT_FREE(p->bevp);
                p->underlying = nullptr; // This should dispose of underlying
                delete p;
            });
    }

    static unsigned long get_openssl_error(Var<Bufferevent> bev) {
        return ::bufferevent_get_openssl_error(bev->bevp);
    }

    Var<Evbuffer> get_input(MKOK_LIBEVENT_MOCKP Var<Bufferevent> bev) {
        return Evbuffer::assign(
            MKOK_LIBEVENT_MOCKP_NAME::bufferevent_get_input(bev->bevp), false);
    }

    Var<Evbuffer> get_output(MKOK_LIBEVENT_MOCKP Var<Bufferevent> bev) {
        return Evbuffer::assign(
            MKOK_LIBEVENT_MOCKP_NAME::bufferevent_get_output(bev->bevp), false);
    }
};

/// @}

#ifdef MKOK_LIBEVENT_NAMESPACE
} // namespace
#endif

extern "C" {

static void mkok_libevent_event_cb(evutil_socket_t, short w, void *p) {
    auto f = static_cast<std::function<void(short)> *>(p);
    (*f)(w);
    delete f;
}

#ifdef MKOK_LIBEVENT_NAMESPACE
#define XX MKOK_LIBEVENT_NAMESPACE::
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
