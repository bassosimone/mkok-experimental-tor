// Part of measurement-kit <https://measurement-kit.github.io/>.
// Measurement-kit is free software. See AUTHORS and LICENSE for more
// information on the copying conditions.

#define SRC_COMMON_LIBEVENT_EVBUFFER_ENABLE_MOCK
#define CATCH_CONFIG_MAIN

#include "src/common/libevent-evbuffer.hpp"
#include "catch.hpp"

using namespace mk;

TEST_CASE("Evbuffer::assign deals with nullptr input") {
    REQUIRE_THROWS_AS(Evbuffer::assign(nullptr, true), NullPointerError);
}

static bool was_called = false;
static void set_was_called(evbuffer *) { was_called = true; }

TEST_CASE("Evbuffer::assign deals with not-owned pointer") {
    REQUIRE(was_called == false);
    Var<Evbuffer> evb = Evbuffer::assign<set_was_called>((evbuffer *)17, false);
    evb = nullptr;
    REQUIRE(was_called == false);
}

static bool was_called2 = false;
static void set_was_called2(evbuffer *) { was_called2 = true; }

TEST_CASE("Evbuffer::assign deals with owned pointer") {
    REQUIRE(was_called2 == false);
    Var<Evbuffer> evb = Evbuffer::assign<set_was_called2>((evbuffer *)17, true);
    evb = nullptr;
    REQUIRE(was_called2 == true);
}

static unsigned char *fail(evbuffer *, ssize_t) { return nullptr; }

TEST_CASE("Evbuffer::pullup deals with evbuffer_pullup failure") {
    Var<Evbuffer> evb = Evbuffer::create();
    REQUIRE_THROWS_AS(evb->pullup<fail>(-1), EvbufferPullupError);
}

static int fail(evbuffer *, size_t) { return -1; }

TEST_CASE("Evbuffer::drain deals with evbuffer_drain failure") {
    Var<Evbuffer> evb = Evbuffer::create();
    REQUIRE_THROWS_AS(evb->drain<fail>(512), EvbufferDrainError);
}

static int success(evbuffer *, size_t) { return 0; }

TEST_CASE("Evbuffer::drain correctly deals with evbuffer_drain success") {
    Var<Evbuffer> evb = Evbuffer::create();
    evb->drain<success>(512);
}

static int fail(evbuffer *, const void *, size_t) { return -1; }

TEST_CASE("Evbuffer::add deails with evbuffer_add failure") {
    Var<Evbuffer> evb = Evbuffer::create();
    REQUIRE_THROWS_AS(evb->add<fail>(nullptr, 0), EvbufferAddError);
}

static int success(evbuffer *, const void *, size_t) { return 0; }

TEST_CASE("Evbuffer::add correctly deails with evbuffer_add success") {
    Var<Evbuffer> evb = Evbuffer::create();
    evb->add<success>(nullptr, 0);
}

static int fail(evbuffer *, evbuffer *) { return -1; }

TEST_CASE("Evbuffer::add_buffer deails with evbuffer_add_buffer failure") {
    Var<Evbuffer> evb = Evbuffer::create();
    Var<Evbuffer> b = Evbuffer::create();
    REQUIRE_THROWS_AS(evb->add_buffer<fail>(b), EvbufferAddBufferError);
}

static int success(evbuffer *, evbuffer *) { return 0; }

TEST_CASE("Evbuffer::add_buffer correctly deails with evbuffer_add_buffer "
          "success") {
    Var<Evbuffer> evb = Evbuffer::create();
    Var<Evbuffer> b = Evbuffer::create();
    evb->add_buffer<success>(b);
}

static int fail(evbuffer *, ssize_t, evbuffer_ptr *, evbuffer_iovec *, int) {
    return -1;
}

TEST_CASE("Evbuffer::peek deals with first evbuffer_peek()'s failure") {
    Var<Evbuffer> evb = Evbuffer::create();
    size_t n_extents = 0;
    REQUIRE_THROWS_AS(evb->peek<fail>(-1, nullptr, n_extents),
                      EvbufferPeekError);
}

static int zero(evbuffer *, ssize_t, evbuffer_ptr *, evbuffer_iovec *, int) {
    return 0;
}

TEST_CASE("Evbuffer::peek deals with evbuffer_peek() returning zero") {
    Var<Evbuffer> evb = Evbuffer::create();
    size_t n_extents = 0;
    auto res = evb->peek<zero>(-1, nullptr, n_extents);
    REQUIRE_THROWS_AS(res.get(), std::runtime_error);
}

static int counter = 17;
static int ret_cnt(evbuffer *, ssize_t, evbuffer_ptr *, evbuffer_iovec *, int) {
    return counter++;
}

TEST_CASE("Evbuffer::peek deals with evbuffer_peek() mismatch") {
    Var<Evbuffer> evb = Evbuffer::create();
    size_t n_extents = 0;
    REQUIRE_THROWS_AS(evb->peek<ret_cnt>(-1, nullptr, n_extents),
                      EvbufferPeekMismatchError);
}

static Var<Evbuffer> fill_with_n_extents(int n) {
    REQUIRE(n > 0);
    Var<Evbuffer> evb = Evbuffer::create();

    // Fill `evb` with at least three extents
    std::string first(512, 'A');
    std::string second(512, 'B');
    std::string third(512, 'C');
    int count = 0;
    do {
        evb->add(first.data(), first.size());
        evb->add(second.data(), second.size());
        evb->add(third.data(), third.size());
    } while ((count = evbuffer_peek(evb->evbuf, -1, nullptr, nullptr, 0)) > 0
             && count < n);
    REQUIRE(count == n);

    return evb;
}

TEST_CASE("Evbuffer::peek works for more than one extent") {
    static const int count = 3;
    Var<Evbuffer> evb = fill_with_n_extents(count);
    size_t n_extents = 0;
    Var<evbuffer_iovec> iov = evb->peek(-1, nullptr, n_extents);
    REQUIRE(iov.get() != nullptr);
    REQUIRE(n_extents == count);
    for (size_t i = 0; i < n_extents; ++i) {
        REQUIRE(iov.get()[i].iov_base != nullptr);
        REQUIRE(iov.get()[i].iov_len > 0);
    }
}

TEST_CASE("Evbuffer::for_each_ deals with peek() returning zero") {
    Var<Evbuffer> evb = Evbuffer::create();
    size_t num = 0;
    evb->for_each_<zero>([&num](const void *, size_t) {
        ++num;
        return true;
    });
    REQUIRE(num == 0);
}

TEST_CASE("Evbuffer::for_ech_ works for more than one extent") {
    static const int count = 3;
    Var<Evbuffer> evb = fill_with_n_extents(count);
    size_t num = 0;
    evb->for_each_([&num](const void *, size_t) {
        ++num;
        return true;
    });
    REQUIRE(num == count);
}

TEST_CASE("Evbuffer::for_each_ can be interrupted earlier") {
    static const int count = 3;
    Var<Evbuffer> evb = fill_with_n_extents(count);
    size_t num = 0;
    evb->for_each_([&num](const void *, size_t) {
        ++num;
        return false;
    });
    REQUIRE(num == 1);
}

TEST_CASE("Evbuffer::copyout works as expected for empty evbuffer") {
    Var<Evbuffer> evb = Evbuffer::create();
    REQUIRE(evb->get_length() == 0);
    REQUIRE(evb->copyout(512) == "");
}

TEST_CASE("Evbuffer::copyout works as expected for evbuffer filled with data") {
    std::string source(4096, 'A');
    Var<Evbuffer> evb = Evbuffer::create();
    evb->add(source.data(), source.size());
    std::string out = evb->copyout(1024);
    REQUIRE(out == std::string(1024, 'A'));
    REQUIRE(evb->get_length() == 4096);
}

TEST_CASE("Evbuffer::remove works as expected for empty evbuffer") {
    Var<Evbuffer> evb = Evbuffer::create();
    REQUIRE(evb->get_length() == 0);
    REQUIRE(evb->remove(512) == "");
}

TEST_CASE("Evbuffer::remove works as expected for evbuffer filled with data") {
    std::string source(4096, 'A');
    Var<Evbuffer> evb = Evbuffer::create();
    evb->add(source.data(), source.size());
    std::string out = evb->remove(1024);
    REQUIRE(out == std::string(1024, 'A'));
    REQUIRE(evb->get_length() == 3072);
}

static int fail(evbuffer *, evbuffer *, size_t) { return -1; }

TEST_CASE("Evbuffer::remove_buffer deals with evbuffer_remove_buffer failure") {
    Var<Evbuffer> evb = Evbuffer::create();
    Var<Evbuffer> b = Evbuffer::create();
    REQUIRE_THROWS_AS(evb->remove_buffer<fail>(b, 512),
                      EvbufferRemoveBufferError);
}

static int success(evbuffer *, evbuffer *, size_t count) {
    return count > 512 ? 512 : count;
}

static void test_evbuffer_remove_buffer_success(int retval) {
    Var<Evbuffer> evb = Evbuffer::create();
    Var<Evbuffer> b = Evbuffer::create();
    REQUIRE(evb->remove_buffer<success>(b, retval)
            == (retval > 512 ? 512 : retval));
}

TEST_CASE("Evbuffer::remove_buffer behaves on evbuffer_remove_buffer success") {
    test_evbuffer_remove_buffer_success(0);
    test_evbuffer_remove_buffer_success(1024);
    test_evbuffer_remove_buffer_success(128);
}

static evbuffer_ptr fail(evbuffer *, evbuffer_ptr *, size_t *,
                         enum evbuffer_eol_style) {
    evbuffer_ptr ptr;
    memset(&ptr, 0, sizeof (ptr));
    ptr.pos = -1;
    return ptr;
}

TEST_CASE("Evbuffer::readln deals with evbuffer_search_eol failure") {
    Var<Evbuffer> evb = Evbuffer::create();
    REQUIRE(evb->readln<fail>(EVBUFFER_EOL_CRLF) == "");
}

TEST_CASE("Evbuffer::readln works") {
    std::string source = "First-line\nSecond-Line\r\nThird-Line incomplete";
    Var<Evbuffer> evb = Evbuffer::create();
    evb->add(source.data(), source.size());
    REQUIRE(evb->readln(EVBUFFER_EOL_CRLF) == "First-line");
    REQUIRE(evb->readln(EVBUFFER_EOL_CRLF) == "Second-Line");
    REQUIRE(evb->readln(EVBUFFER_EOL_CRLF) == "");
    // The -1 is because of the final '\0'
    REQUIRE(evb->get_length() == sizeof("Third-Line incomplete") - 1);
}
