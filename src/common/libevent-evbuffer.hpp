// Part of measurement-kit <https://measurement-kit.github.io/>.
// Measurement-kit is free software. See AUTHORS and LICENSE for more
// information on the copying conditions.

#ifndef SRC_COMMON_LIBEVENT_EVBUFFER_HPP
#define SRC_COMMON_LIBEVENT_EVBUFFER_HPP

#include <arpa/inet.h>
#include <cstddef>
#include <event2/buffer.h>
#include <functional>
#include <measurement_kit/common/error.hpp>
#include <measurement_kit/common/var.hpp>
#include <sys/types.h>
#include <sys/uio.h>
#include <string>

// Forward declarations
struct evbuffer;

namespace mk {

#ifdef SRC_COMMON_LIBEVENT_EVBUFFER_ENABLE_MOCK

class EvbufferMock {
  public:
#define XX(name, signature) std::function<signature> name = ::name
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
#undef XX
};

#define MockPtrArg EvbufferMock *mockp,
#define MockPtrArg0 EvbufferMock *mockp
#define MockPtrName mockp,
#define call(func, ...) mockp->func(__VA_ARGS__)

#else

#define MockPtrArg
#define MockPtrArg0
#define MockPtrName
#define call(func, ...) ::func(__VA_ARGS__)

#endif

class Evbuffer {
  public:
    evbuffer *evbuf = nullptr;
    bool owned = false;
#ifdef SRC_COMMON_LIBEVENT_EVBUFFER_ENABLE_MOCK
    EvbufferMock *mockp = nullptr;
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
            MK_THROW(NullPointerError);
        }
        Var<Evbuffer> evbuf(new Evbuffer);
        evbuf->evbuf = pointer;
        evbuf->owned = owned;
#ifdef SRC_COMMON_LIBEVENT_EVBUFFER_ENABLE_MOCK
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
            MK_THROW(EvbufferPullupError);
        }
        return std::string((char *)s, get_length(evbuf));
    }

    static void drain(MockPtrArg Var<Evbuffer> evbuf, size_t n) {
        if (call(evbuffer_drain, evbuf->evbuf, n) != 0) {
            MK_THROW(EvbufferDrainError);
        }
    }

    static void add(MockPtrArg Var<Evbuffer> evbuf, const void *base,
                    size_t count) {
        if (call(evbuffer_add, evbuf->evbuf, base, count) != 0) {
            MK_THROW(EvbufferAddError);
        }
    }

    static void add_buffer(MockPtrArg Var<Evbuffer> evbuf, Var<Evbuffer> b) {
        if (call(evbuffer_add_buffer, evbuf->evbuf, b->evbuf) != 0) {
            MK_THROW(EvbufferAddBufferError);
        }
    }

    static Var<evbuffer_iovec> peek(MockPtrArg Var<Evbuffer> evbuf, ssize_t len,
                                    evbuffer_ptr *start_at, size_t &n_extents) {
        int required =
            call(evbuffer_peek, evbuf->evbuf, len, start_at, nullptr, 0);
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
        int used =
            call(evbuffer_peek, evbuf->evbuf, len, start_at, iov, required);
        if (used != required) {
            MK_THROW(EvbufferPeekMismatchError);
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
            MK_THROW(EvbufferRemoveBufferError);
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

    static void add_uint8(MockPtrArg Var<Evbuffer> evbuf, uint8_t num) {
        add(MockPtrName evbuf, &num, sizeof (num));
    }

    static void add_uint16(MockPtrArg Var<Evbuffer> evbuf, uint16_t num) {
        num = htons(num);
        add(MockPtrName evbuf, &num, sizeof(num));
    }

    static void add_uint32(MockPtrArg Var<Evbuffer> evbuf, uint32_t num) {
        num = htonl(num);
        add(MockPtrName evbuf, &num, sizeof(num));
    }
};

// Undefine internal macros
#undef MockPtrArg
#undef MockPtrArg0
#undef MockPtrName
#undef call

} // namespace mk
#endif
