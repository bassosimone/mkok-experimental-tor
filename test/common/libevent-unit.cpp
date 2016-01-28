// Part of measurement-kit <https://measurement-kit.github.io/>.
// Measurement-kit is free software. See AUTHORS and LICENSE for more
// information on the copying conditions.

#define SRC_COMMON_LIBEVENT_ENABLE_MOCK
#define CATCH_CONFIG_MAIN

#include "src/common/libevent.hpp"
#include "catch.hpp"

using namespace mk;

// Evutil

static int success(evutil_socket_t) { return 0; }
static int fail(evutil_socket_t) { return -1; }

TEST_CASE("We deal with evutil_make_socket_nonblocking success") {
    evutil::make_socket_nonblocking<success>(0);
}

TEST_CASE("We deal with evutil_make_socket_nonblocking failure") {
    REQUIRE_THROWS_AS(evutil::make_socket_nonblocking<fail>(0),
                      EvutilMakeSocketNonblockingError);
}

static int success(const char *, sockaddr *, int *) { return 0; }
static int fail(const char *, sockaddr *, int *) { return -1; }

TEST_CASE("We deal with evutil_parse_sockaddr_port success") {
    REQUIRE(evutil::parse_sockaddr_port<success>("", nullptr, nullptr) == 0);
}

TEST_CASE("We deal with evutil_parse_sockaddr_port failure") {
    REQUIRE(evutil::parse_sockaddr_port<fail>("", nullptr, nullptr) ==
            EvutilParseSockaddrPortError());
}

TEST_CASE("We deal with evutil_make_listen_socket_reuseable success") {
    evutil::make_listen_socket_reuseable<success>(0);
}

TEST_CASE("We deal with evutil_make_listen_socket_reuseable failure") {
    REQUIRE_THROWS_AS(evutil::make_listen_socket_reuseable<fail>(0),
                      EvutilMakeListenSocketReuseableError);
}

// EventBase

TEST_CASE("EventBase default constructor works") {
    EventBase evbase;
    REQUIRE(evbase.evbase == nullptr);
    REQUIRE(evbase.owned == false);
}

static bool destructor_was_called = false;
static void record_if_called(event_base *) { destructor_was_called = true; }

TEST_CASE("EventBase does not call destructor if not owned") {
    REQUIRE(destructor_was_called == false);
    Var<EventBase> evbase =
        EventBase::assign<record_if_called>((event_base *)128, false);
    REQUIRE(destructor_was_called == false);
}

TEST_CASE("EventBase does not call destructor if owned but evbase is null") {
    REQUIRE(destructor_was_called == false);
    // We cannot directly assign nullptr because of a check in assign():
    Var<EventBase> evbase =
        EventBase::assign<record_if_called>((event_base *)128, true);
    evbase->evbase = nullptr;
    REQUIRE(destructor_was_called == false);
}

TEST_CASE("EventBase calls destructor if owned and evbase is not null") {
    REQUIRE(destructor_was_called == false);
    []() { EventBase::assign<record_if_called>((event_base *)128, true); }();
    REQUIRE(destructor_was_called == true);
}

TEST_CASE("EventBase::assign throws if passed a nullptr") {
    REQUIRE_THROWS_AS(EventBase::assign(nullptr, true), NullPointerError);
}

static event_base *evbase_fail() { return nullptr; }

TEST_CASE("EventBase::create deals with event_base_new() failure") {
    REQUIRE_THROWS_AS(EventBase::create<evbase_fail>(), NullPointerError);
}

static int return_zero(event_base *) { return 0; }
static int return_one(event_base *) { return 1; }
static int fail(event_base *) { return -1; }

TEST_CASE("EventBase::dispatch deals with event_base_dispatch() returning 0") {
    Var<EventBase> evb = EventBase::create();
    evb->dispatch<return_zero>();
}

TEST_CASE("EventBase::dispatch deals with event_base_dispatch() returning 1") {
    Var<EventBase> evb = EventBase::create();
    evb->dispatch<return_one>();
}

TEST_CASE("EventBase::dispatch deals with event_base_dispatch() failure") {
    Var<EventBase> evb = EventBase::create();
    REQUIRE_THROWS_AS(evb->dispatch<fail>(), EventBaseDispatchError);
}

static int return_zero(event_base *, int) { return 0; }
static int return_one(event_base *, int) { return 1; }
static int fail(event_base *, int) { return -1; }

TEST_CASE("EventBase::loop deals with event_base_loop() returning 0") {
    Var<EventBase> evb = EventBase::create();
    evb->loop<return_zero>(0);
}

TEST_CASE("EventBase::loop deals with event_base_loop() returning 1") {
    Var<EventBase> evb = EventBase::create();
    evb->loop<return_one>(0);
}

TEST_CASE("EventBase::loop deals with event_base_loop() failure") {
    Var<EventBase> evb = EventBase::create();
    REQUIRE_THROWS_AS(evb->loop<fail>(0), EventBaseLoopError);
}

TEST_CASE("EventBase::loopbreak deals with event_base_loopbreak() failure") {
    Var<EventBase> evb = EventBase::create();
    REQUIRE_THROWS_AS(evb->loopbreak<fail>(), EventBaseLoopbreakError);
}

static int fail(event_base *, evutil_socket_t, short, event_callback_fn, void *,
                const timeval *) {
    return -1;
};

TEST_CASE("EventBase::once deals with event_base_once() failure") {
    Var<EventBase> evb = EventBase::create();
    REQUIRE_THROWS_AS(evb->once<fail>(0, EV_TIMEOUT, nullptr, nullptr),
                      EventBaseOnceError);
}

// Evbuffer

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
    } while ((count = evbuffer_peek(evb->evbuf, -1, nullptr, nullptr, 0)) > 0 &&
             count < n);
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
    REQUIRE(evb->remove_buffer<success>(b, retval) ==
            (retval > 512 ? 512 : retval));
}

TEST_CASE("Evbuffer::remove_buffer behaves on evbuffer_remove_buffer success") {
    test_evbuffer_remove_buffer_success(0);
    test_evbuffer_remove_buffer_success(1024);
    test_evbuffer_remove_buffer_success(128);
}

static evbuffer_ptr fail(evbuffer *, evbuffer_ptr *, size_t *,
                         enum evbuffer_eol_style) {
    evbuffer_ptr ptr;
    memset(&ptr, 0, sizeof(ptr));
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

// Bufferevent

TEST_CASE("Bufferevent::event_string works") {
// Only test the (in my opinion) most common flags
#define XX(flags, string) REQUIRE(Bufferevent::event_string(flags) == string)
    XX(BEV_EVENT_READING, "reading ");
    XX(BEV_EVENT_WRITING, "writing ");
    XX(BEV_EVENT_EOF, "eof ");
    XX(BEV_EVENT_ERROR, "error ");
    XX(BEV_EVENT_TIMEOUT, "timeout ");
    XX(BEV_EVENT_CONNECTED, "connected ");
    XX(BEV_EVENT_READING | BEV_EVENT_EOF, "reading eof ");
    XX(BEV_EVENT_WRITING | BEV_EVENT_EOF, "writing eof ");
    XX(BEV_EVENT_READING | BEV_EVENT_ERROR, "reading error ");
    XX(BEV_EVENT_WRITING | BEV_EVENT_ERROR, "writing error ");
    XX(BEV_EVENT_READING | BEV_EVENT_TIMEOUT, "reading timeout ");
    XX(BEV_EVENT_WRITING | BEV_EVENT_TIMEOUT, "writing timeout ");
    XX(BEV_EVENT_CONNECTED | BEV_EVENT_TIMEOUT, "connected timeout ");
    XX(BEV_EVENT_CONNECTED | BEV_EVENT_ERROR, "connected error ");
#undef XX
}

static bufferevent *fail(event_base *, evutil_socket_t, int) { return nullptr; }

TEST_CASE("Bufferevent::socket_new deals with bufferevent_socket_new failure") {
    REQUIRE_THROWS_AS(Bufferevent::socket_new<fail>(EventBase::create(), -1, 0),
                      BuffereventSocketNewError);
}

// This function ensures that the Bufferevent allocated by `socket_new` is
// properly destroyed once done, by removing all the references to it
static void with_bufferevent(std::function<void(Var<Bufferevent>)> cb) {
    Var<EventBase> evbase = EventBase::create();
    Var<Bufferevent> bufev = Bufferevent::socket_new(evbase, -1, 0);
    cb(bufev);
    delete bufev->the_opaque;
    bufev->the_opaque = nullptr;
    bufev = nullptr;
}

static int fail(bufferevent *, sockaddr *, int) { return -1; }

TEST_CASE("Bufferevent::socket_connect deals with bufferevent_socket_connect "
          "failure") {
    with_bufferevent([](Var<Bufferevent> bufev) {
        REQUIRE_THROWS_AS(bufev->socket_connect<fail>(nullptr, 0),
                          BuffereventSocketConnectError);
    });
}

static int fail(bufferevent *, const void *, size_t) { return -1; }

TEST_CASE("Bufferevent::write deals with bufferevent_write failure") {
    with_bufferevent([](Var<Bufferevent> bufev) {
        REQUIRE_THROWS_AS(bufev->write<fail>(nullptr, 0),
                          BuffereventWriteError);
    });
}

static int fail(bufferevent *, evbuffer *) { return -1; }

TEST_CASE("Bufferevent::write_buffer deals with bufferevent_write_buffer "
          "failure") {
    with_bufferevent([](Var<Bufferevent> bufev) {
        Var<Evbuffer> evbuf = Evbuffer::create();
        REQUIRE_THROWS_AS(bufev->write_buffer<fail>(evbuf),
                          BuffereventWriteBufferError);
    });
}

static int success(bufferevent *, evbuffer *) { return 0; }

TEST_CASE("Bufferevent::write_buffer behaves on bufferevent_write_buffer "
          "success") {
    with_bufferevent([](Var<Bufferevent> bufev) {
        Var<Evbuffer> evbuf = Evbuffer::create();
        bufev->write_buffer<success>(evbuf);
    });
}

TEST_CASE("Bufferevent::read_buffer deals with bufferevent_read_buffer "
          "failure") {
    with_bufferevent([](Var<Bufferevent> bufev) {
        Var<Evbuffer> evbuf = Evbuffer::create();
        REQUIRE_THROWS_AS(bufev->read_buffer<fail>(evbuf),
                          BuffereventReadBufferError);
    });
}

static int fail(bufferevent *, short) { return -1; }

TEST_CASE("Bufferevent::enable deals with bufferevent_enable failure") {
    with_bufferevent([](Var<Bufferevent> bufev) {
        REQUIRE_THROWS_AS(bufev->enable<fail>(EV_READ), BuffereventEnableError);
    });
}

static int success(bufferevent *, short) { return 0; }

TEST_CASE("Bufferevent::enable deals with successful bufferevent_enable") {
    with_bufferevent(
        [](Var<Bufferevent> bufev) { bufev->enable<success>(EV_READ); });
}

TEST_CASE("Bufferevent::disable deals with bufferevent_disable failure") {
    with_bufferevent([](Var<Bufferevent> bufev) {
        REQUIRE_THROWS_AS(bufev->disable<fail>(EV_READ),
                          BuffereventDisableError);
    });
}

TEST_CASE("Bufferevent::disable deals with successful bufferevent_disable") {
    with_bufferevent(
        [](Var<Bufferevent> bufev) { bufev->disable<success>(EV_READ); });
}

static int fail(bufferevent *, const timeval *, const timeval *) { return -1; }

TEST_CASE("Bufferevent::set_timeouts deals with bufferevent_set_timeouts "
          "failure") {
    with_bufferevent([](Var<Bufferevent> bufev) {
        REQUIRE_THROWS_AS(bufev->set_timeouts<fail>(nullptr, nullptr),
                          BuffereventSetTimeoutsError);
    });
}

static bufferevent *fail(event_base *, bufferevent *, ssl_st *,
                         enum bufferevent_ssl_state, int) {
    return nullptr;
}

TEST_CASE("Bufferevent::openssl_filter_new deals with "
          "bufferevent_openssl_filter_new failure") {
    with_bufferevent([](Var<Bufferevent> bufev) {
        REQUIRE_THROWS_AS(
            Bufferevent::openssl_filter_new<fail>(bufev->evbase, bufev, nullptr,
                                                  BUFFEREVENT_SSL_OPEN, 0),
            BuffereventOpensslFilterNewError);
    });
}

TEST_CASE("Bufferevent::get_input returns evbuffer not owning pointer") {
    with_bufferevent([](Var<Bufferevent> bufev) {
        Var<Evbuffer> evbuf = bufev->get_input();
        REQUIRE(evbuf->owned == false);
    });
}

TEST_CASE("Bufferevent::get_output returns evbuffer not owning pointer") {
    with_bufferevent([](Var<Bufferevent> bufev) {
        Var<Evbuffer> evbuf = bufev->get_output();
        REQUIRE(evbuf->owned == false);
    });
}
