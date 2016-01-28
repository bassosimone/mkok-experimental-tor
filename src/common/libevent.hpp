// Part of measurement-kit <https://measurement-kit.github.io/>.
// Measurement-kit is free software. See AUTHORS and LICENSE for more
// information on the copying conditions.

#ifndef SRC_COMMON_LIBEVENT_HPP
#define SRC_COMMON_LIBEVENT_HPP

#include <arpa/inet.h>
#include <cstddef>
#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <event2/bufferevent_ssl.h>
#include <event2/event.h>
#include <event2/util.h>
#include <exception>
#include <functional>
#include <memory>
#include <measurement_kit/common/constraints.hpp>
#include <measurement_kit/common/error.hpp>
#include <measurement_kit/common/func.hpp>
#include <measurement_kit/common/var.hpp>
#include <string>
#include <sys/types.h>
#include <sys/uio.h>
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

void mk_libevent_bev_read(bufferevent *, void *ptr);
void mk_libevent_bev_write(bufferevent *, void *ptr);
void mk_libevent_bev_event(bufferevent *, short what, void *ptr);
void mk_libevent_event_cb(evutil_socket_t, short w, void *p);

} // extern "C"

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

class EventBase : public NonCopyable, public NonMovable {
  public:
    event_base *evbase = nullptr;
    bool owned = false;

    EventBase() {}
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

class Evbuffer : public NonCopyable, public NonMovable {
  public:
    evbuffer *evbuf = nullptr;
    bool owned = false;

    Evbuffer() {}
    ~Evbuffer() {}

    template <void (*func)(evbuffer *) = ::evbuffer_free>
    static Var<Evbuffer> assign(evbuffer *pointer, bool owned) {
        if (pointer == nullptr) {
            MK_THROW(NullPointerError);
        }
        Var<Evbuffer> evbuf(new Evbuffer, [](Evbuffer *ptr) {
            if (ptr->owned && ptr->evbuf != nullptr) {
                func(ptr->evbuf);
                ptr->owned = false;
                ptr->evbuf = nullptr;
            }
            delete ptr;
        });
        evbuf->evbuf = pointer;
        evbuf->owned = owned;
        return evbuf;
    }

    template <evbuffer *(*construct)() = ::evbuffer_new,
              void (*destruct)(evbuffer *) = ::evbuffer_free>
    static Var<Evbuffer> create() {
        return assign<destruct>(construct(), true);
    }

    size_t get_length() { return ::evbuffer_get_length(evbuf); }

    template <unsigned char *(*func)(evbuffer *, ssize_t) = ::evbuffer_pullup>
    std::string pullup(ssize_t n) {
        unsigned char *s = func(evbuf, n);
        if (s == nullptr) {
            MK_THROW(EvbufferPullupError);
        }
        return std::string((char *)s, get_length());
    }

    template <int (*func)(evbuffer *, size_t) = ::evbuffer_drain>
    void drain(size_t n) {
        if (func(evbuf, n) != 0) {
            MK_THROW(EvbufferDrainError);
        }
    }

    template <int (*func)(evbuffer *, const void *, size_t) = ::evbuffer_add>
    void add(const void *base, size_t count) {
        if (func(evbuf, base, count) != 0) {
            MK_THROW(EvbufferAddError);
        }
    }

    template <int (*func)(evbuffer *, evbuffer *) = ::evbuffer_add_buffer>
    void add_buffer(Var<Evbuffer> b) {
        if (func(evbuf, b->evbuf) != 0) {
            MK_THROW(EvbufferAddBufferError);
        }
    }

    template <int (*func)(evbuffer *, ssize_t, evbuffer_ptr *, evbuffer_iovec *,
                          int) = ::evbuffer_peek>
    Var<evbuffer_iovec> peek(ssize_t len, evbuffer_ptr *start_at,
                             size_t &n_extents) {
        int required = func(evbuf, len, start_at, nullptr, 0);
        if (required < 0) {
            MK_THROW(EvbufferPeekError);
        }
        if (required == 0) {
            n_extents = (unsigned)required;
            return nullptr; // Caller required to check return value
        }
        Var<evbuffer_iovec> retval(new evbuffer_iovec[required],
                                   [](evbuffer_iovec *p) { delete[] p; });
        evbuffer_iovec *iov = retval.get();
        int used = func(evbuf, len, start_at, iov, required);
        if (used != required) {
            MK_THROW(EvbufferPeekMismatchError);
        }
        // Cast to unsigned safe because we excluded negative case above
        n_extents = (unsigned)required;
        return retval;
    }

    template <int (*func)(evbuffer *, ssize_t, evbuffer_ptr *, evbuffer_iovec *,
                          int) = ::evbuffer_peek>
    void for_each_(std::function<bool(const void *, size_t)> cb) {
        // Not part of libevent API but useful to wrap part of such API
        size_t n_extents = 0;
        Var<evbuffer_iovec> raii = peek<func>(-1, nullptr, n_extents);
        if (!raii) return; // Prevent exception if pointer is null
        auto iov = raii.get();
        for (size_t i = 0; i < n_extents; ++i) {
            if (!cb(iov[i].iov_base, iov[i].iov_len)) {
                break;
            }
        }
    }

    std::string copyout(size_t upto) {
        std::string out;
        for_each_([&out, &upto](const void *p, size_t n) {
            if (upto < n) n = upto;
            out.append((const char *)p, n);
            upto -= n;
            return (upto > 0);
        });
        return out;
    }

    std::string remove(size_t upto) {
        std::string out = copyout(upto);
        if (out.size() > 0) {
            drain(out.size());
        }
        return out;
    }

    template <int (*func)(evbuffer *, evbuffer *,
                          size_t) = ::evbuffer_remove_buffer>
    int remove_buffer(Var<Evbuffer> b, size_t count) {
        int len = func(evbuf, b->evbuf, count);
        if (len < 0) {
            MK_THROW(EvbufferRemoveBufferError);
        }
        return len;
    }

    template <
        evbuffer_ptr (*func)(evbuffer *, evbuffer_ptr *, size_t *,
                             enum evbuffer_eol_style) = ::evbuffer_search_eol>
    std::string readln(enum evbuffer_eol_style style) {
        size_t eol_length = 0;
        auto sre = func(evbuf, nullptr, &eol_length, style);
        if (sre.pos < 0) {
            return "";
        }
        // Note: pos is ssize_t and we have excluded the negative case above
        std::string out = remove((size_t)sre.pos);
        drain(eol_length);
        return out;
    }

    void add_uint8(uint8_t num) { add(&num, sizeof(num)); }

    void add_uint16(uint16_t num) {
        num = htons(num);
        add(&num, sizeof(num));
    }

    void add_uint32(uint32_t num) {
        num = htonl(num);
        add(&num, sizeof(num));
    }
};

class Bufferevent : public NonCopyable, public NonMovable {
  public:
    Func<void(short)> event_cb;
    Func<void()> read_cb;
    Func<void()> write_cb;
    bufferevent *bevp = nullptr;
    Var<EventBase> evbase;
#ifdef SRC_COMMON_LIBEVENT_ENABLE_MOCK
    Var<Bufferevent> *the_opaque = nullptr; // To avoid leak in regress tests
#endif

    Bufferevent() {}
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
#ifdef SRC_COMMON_LIBEVENT_ENABLE_MOCK
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
#ifdef SRC_COMMON_LIBEVENT_ENABLE_MOCK
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
