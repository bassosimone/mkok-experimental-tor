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

class Evbuffer {
  public:
    evbuffer *evbuf = nullptr;
    bool owned = false;

    Evbuffer() {}
    Evbuffer(Evbuffer &) = delete;
    Evbuffer &operator=(Evbuffer &) = delete;
    Evbuffer(Evbuffer &&) = delete;
    Evbuffer &operator=(Evbuffer &&) = delete;
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

} // namespace mk
#endif
