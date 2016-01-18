// Part of measurement-kit <https://measurement-kit.github.io/>.
// Measurement-kit is free software. See AUTHORS and LICENSE for more
// information on the copying conditions.

#define SRC_COMMON_LIBEVENT_EVBUFFER_ENABLE_MOCK
#define CATCH_CONFIG_MAIN

#include "src/common/libevent-evbuffer.hpp"
#include "catch.hpp"

using namespace mk;

TEST_CASE("Evbuffer::assign deals with nullptr input") {
    EvbufferMock mock;
    REQUIRE_THROWS_AS(Evbuffer::assign(&mock, nullptr, true),
                      NullPointerError);
}

TEST_CASE("Evbuffer::assign deals with not-owned pointer") {
    EvbufferMock mock;
    bool called = false;
    mock.evbuffer_free = [&called](evbuffer *) { called = true; };
    Var<Evbuffer> evb = Evbuffer::assign(&mock, (evbuffer *)17, false);
    evb = nullptr;
    REQUIRE(called == false);
}

TEST_CASE("Evbuffer::assign correctly sets mock") {
    bool called = false;
    EvbufferMock mock;
    mock.evbuffer_free = [&called](evbuffer *) { called = true; };
    Var<Evbuffer> evb = Evbuffer::assign(&mock, (evbuffer *)17, true);
    evb = nullptr;
    REQUIRE(called == true);
}

TEST_CASE("Evbuffer::pullup deals with evbuffer_pullup failure") {
    EvbufferMock mock;
    mock.evbuffer_pullup = [](evbuffer *, ssize_t) {
        return (unsigned char *)nullptr;
    };
    Var<Evbuffer> evb = Evbuffer::create(&mock);
    REQUIRE_THROWS_AS(Evbuffer::pullup(&mock, evb, -1),
                      EvbufferPullupError);
}

TEST_CASE("Evbuffer::drain deals with evbuffer_drain failure") {
    EvbufferMock mock;
    mock.evbuffer_drain = [](evbuffer *, size_t) { return -1; };
    Var<Evbuffer> evb = Evbuffer::create(&mock);
    REQUIRE_THROWS_AS(Evbuffer::drain(&mock, evb, 512), EvbufferDrainError);
}

TEST_CASE("Evbuffer::drain correctly deals with evbuffer_drain success") {
    EvbufferMock mock;
    mock.evbuffer_drain = [](evbuffer *, size_t) { return 0; };
    Var<Evbuffer> evb = Evbuffer::create(&mock);
    Evbuffer::drain(&mock, evb, 512);
}

TEST_CASE("Evbuffer::add deails with evbuffer_add failure") {
    EvbufferMock mock;
    mock.evbuffer_add = [](evbuffer *, const void *, size_t) { return -1; };
    Var<Evbuffer> evb = Evbuffer::create(&mock);
    REQUIRE_THROWS_AS(Evbuffer::add(&mock, evb, nullptr, 0),
                      EvbufferAddError);
}

TEST_CASE("Evbuffer::add correctly deails with evbuffer_add success") {
    EvbufferMock mock;
    mock.evbuffer_add = [](evbuffer *, const void *, size_t) { return 0; };
    Var<Evbuffer> evb = Evbuffer::create(&mock);
    Evbuffer::add(&mock, evb, nullptr, 0);
}

TEST_CASE("Evbuffer::add_buffer deails with evbuffer_add_buffer failure") {
    EvbufferMock mock;
    mock.evbuffer_add_buffer = [](evbuffer *, evbuffer *) { return -1; };
    Var<Evbuffer> evb = Evbuffer::create(&mock);
    Var<Evbuffer> b = Evbuffer::create(&mock);
    REQUIRE_THROWS_AS(Evbuffer::add_buffer(&mock, evb, b),
                      EvbufferAddBufferError);
}

TEST_CASE("Evbuffer::add_buffer correctly deails with evbuffer_add_buffer "
          "success") {
    EvbufferMock mock;
    mock.evbuffer_add_buffer = [](evbuffer *, evbuffer *) { return 0; };
    Var<Evbuffer> evb = Evbuffer::create(&mock);
    Var<Evbuffer> b = Evbuffer::create(&mock);
    Evbuffer::add_buffer(&mock, evb, b);
}

TEST_CASE("Evbuffer::peek deals with first evbuffer_peek()'s failure") {
    EvbufferMock mock;
    mock.evbuffer_peek =
        [](evbuffer *, ssize_t, evbuffer_ptr *, evbuffer_iovec *, int) {
            return -1;
        };
    Var<Evbuffer> evb = Evbuffer::create(&mock);
    size_t n_extents = 0;
    REQUIRE_THROWS_AS(Evbuffer::peek(&mock, evb, -1, nullptr, n_extents),
                      EvbufferPeekError);
}

TEST_CASE("Evbuffer::peek deals with evbuffer_peek() returning zero") {
    EvbufferMock mock;
    mock.evbuffer_peek =
        [](evbuffer *, ssize_t, evbuffer_ptr *, evbuffer_iovec *, int) {
            return 0;
        };
    Var<Evbuffer> evb = Evbuffer::create(&mock);
    size_t n_extents = 0;
    auto res = Evbuffer::peek(&mock, evb, -1, nullptr, n_extents);
    REQUIRE_THROWS_AS(res.get(), std::runtime_error);
}

TEST_CASE("Evbuffer::peek deals with evbuffer_peek() mismatch") {
    auto count = 17;
    EvbufferMock mock;
    mock.evbuffer_peek =
        [&count](evbuffer *, ssize_t, evbuffer_ptr *, evbuffer_iovec *, int) {
            return count++;
        };
    Var<Evbuffer> evb = Evbuffer::create(&mock);
    size_t n_extents = 0;
    REQUIRE_THROWS_AS(Evbuffer::peek(&mock, evb, -1, nullptr, n_extents),
                      EvbufferPeekMismatchError);
}

static Var<Evbuffer> fill_with_n_extents(EvbufferMock *mock, int n) {
    REQUIRE(n > 0);
    Var<Evbuffer> evb = Evbuffer::create(mock);

    // Fill `evb` with at least three extents
    std::string first(512, 'A');
    std::string second(512, 'B');
    std::string third(512, 'C');
    int count = 0;
    do {
        Evbuffer::add(mock, evb, first.data(), first.size());
        Evbuffer::add(mock, evb, second.data(), second.size());
        Evbuffer::add(mock, evb, third.data(), third.size());
    } while ((count = evbuffer_peek(evb->evbuf, -1, nullptr, nullptr, 0)) > 0
             && count < n);
    REQUIRE(count == n);

    return evb;
}

TEST_CASE("Evbuffer::peek works for more than one extent") {
    static const int count = 3;
    EvbufferMock mock;
    Var<Evbuffer> evb = fill_with_n_extents(&mock, count);
    size_t n_extents = 0;
    Var<evbuffer_iovec> iov = Evbuffer::peek(&mock, evb, -1, nullptr,
                                             n_extents);
    REQUIRE(iov.get() != nullptr);
    REQUIRE(n_extents == count);
    for (size_t i = 0; i < n_extents; ++i) {
        REQUIRE(iov.get()[i].iov_base != nullptr);
        REQUIRE(iov.get()[i].iov_len > 0);
    }
}

TEST_CASE("Evbuffer::for_each_ deals with peek() returning zero") {
    EvbufferMock mock;
    mock.evbuffer_peek =
        [](evbuffer *, ssize_t, evbuffer_ptr *, evbuffer_iovec *, int) {
            return 0;
        };
    Var<Evbuffer> evb = Evbuffer::create(&mock);
    size_t num = 0;
    Evbuffer::for_each_(&mock, evb, [&num](const void *, size_t) {
        ++num;
        return true;
    });
    REQUIRE(num == 0);
}

TEST_CASE("Evbuffer::for_ech_ works for more than one extent") {
    static const int count = 3;
    EvbufferMock mock;
    Var<Evbuffer> evb = fill_with_n_extents(&mock, count);
    size_t num = 0;
    Evbuffer::for_each_(&mock, evb, [&num](const void *, size_t) {
        ++num;
        return true;
    });
    REQUIRE(num == count);
}

TEST_CASE("Evbuffer::for_each_ can be interrupted earlier") {
    static const int count = 3;
    EvbufferMock mock;
    Var<Evbuffer> evb = fill_with_n_extents(&mock, count);
    size_t num = 0;
    Evbuffer::for_each_(&mock, evb, [&num](const void *, size_t) {
        ++num;
        return false;
    });
    REQUIRE(num == 1);
}

TEST_CASE("Evbuffer::copyout works as expected for empty evbuffer") {
    EvbufferMock mock;
    Var<Evbuffer> evb = Evbuffer::create(&mock);
    REQUIRE(Evbuffer::get_length(evb) == 0);
    REQUIRE(Evbuffer::copyout(&mock, evb, 512) == "");
}

TEST_CASE("Evbuffer::copyout works as expected for evbuffer filled with data") {
    EvbufferMock mock;
    std::string source(4096, 'A');
    Var<Evbuffer> evb = Evbuffer::create(&mock);
    Evbuffer::add(&mock, evb, source.data(), source.size());
    std::string out = Evbuffer::copyout(&mock, evb, 1024);
    REQUIRE(out == std::string(1024, 'A'));
    REQUIRE(Evbuffer::get_length(evb) == 4096);
}

TEST_CASE("Evbuffer::remove works as expected for empty evbuffer") {
    EvbufferMock mock;
    Var<Evbuffer> evb = Evbuffer::create(&mock);
    REQUIRE(Evbuffer::get_length(evb) == 0);
    REQUIRE(Evbuffer::remove(&mock, evb, 512) == "");
}

TEST_CASE("Evbuffer::remove works as expected for evbuffer filled with data") {
    EvbufferMock mock;
    std::string source(4096, 'A');
    Var<Evbuffer> evb = Evbuffer::create(&mock);
    Evbuffer::add(&mock, evb, source.data(), source.size());
    std::string out = Evbuffer::remove(&mock, evb, 1024);
    REQUIRE(out == std::string(1024, 'A'));
    REQUIRE(Evbuffer::get_length(evb) == 3072);
}

TEST_CASE("Evbuffer::remove_buffer deals with evbuffer_remove_buffer failure") {
    EvbufferMock mock;
    mock.evbuffer_remove_buffer = [](evbuffer *, evbuffer *, size_t) {
        return -1;
    };
    Var<Evbuffer> evb = Evbuffer::create(&mock);
    Var<Evbuffer> b = Evbuffer::create(&mock);
    REQUIRE_THROWS_AS(Evbuffer::remove_buffer(&mock, evb, b, 512),
                      EvbufferRemoveBufferError);
}

static void test_evbuffer_remove_buffer_success(int retval) {
    EvbufferMock mock;
    mock.evbuffer_remove_buffer = [](evbuffer *, evbuffer *, size_t count) {
        return count > 512 ? 512 : count;
    };
    Var<Evbuffer> evb = Evbuffer::create(&mock);
    Var<Evbuffer> b = Evbuffer::create(&mock);
    REQUIRE(Evbuffer::remove_buffer(&mock, evb, b, retval)
            == (retval > 512 ? 512 : retval));
}

TEST_CASE("Evbuffer::remove_buffer behaves on evbuffer_remove_buffer success") {
    test_evbuffer_remove_buffer_success(0);
    test_evbuffer_remove_buffer_success(1024);
    test_evbuffer_remove_buffer_success(128);
}

TEST_CASE("Evbuffer::readln deals with evbuffer_search_eol failure") {
    EvbufferMock mock;
    mock.evbuffer_search_eol = [](evbuffer *, evbuffer_ptr *, size_t *,
                                  enum evbuffer_eol_style) {
        evbuffer_ptr ptr;
        memset(&ptr, 0, sizeof (ptr));
        ptr.pos = -1;
        return ptr;
    };
    Var<Evbuffer> evb = Evbuffer::create(&mock);
    REQUIRE(Evbuffer::readln(&mock, evb, EVBUFFER_EOL_CRLF) == "");
}

TEST_CASE("Evbuffer::readln works") {
    EvbufferMock mock;
    std::string source = "First-line\nSecond-Line\r\nThird-Line incomplete";
    Var<Evbuffer> evb = Evbuffer::create(&mock);
    Evbuffer::add(&mock, evb, source.data(), source.size());
    REQUIRE(Evbuffer::readln(&mock, evb, EVBUFFER_EOL_CRLF)
            == "First-line");
    REQUIRE(Evbuffer::readln(&mock, evb, EVBUFFER_EOL_CRLF)
            == "Second-Line");
    REQUIRE(Evbuffer::readln(&mock, evb, EVBUFFER_EOL_CRLF) == "");
    // The -1 is because of the final '\0'
    REQUIRE(Evbuffer::get_length(evb) == sizeof("Third-Line incomplete") - 1);
}
