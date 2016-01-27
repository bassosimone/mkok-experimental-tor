// Part of measurement-kit <https://measurement-kit.github.io/>.
// Measurement-kit is free software. See AUTHORS and LICENSE for more
// information on the copying conditions.

#define SRC_COMMON_LIBEVENT_CORE_ENABLE_MOCK
#define CATCH_CONFIG_MAIN

#include "src/common/libevent-core.hpp"
#include "catch.hpp"

using namespace mk;

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
    Var<EventBase> evbase = EventBase::assign<record_if_called>(
        (event_base *) 128, false);
    REQUIRE(destructor_was_called == false);
}

TEST_CASE("EventBase does not call destructor if owned but evbase is null") {
    REQUIRE(destructor_was_called == false);
    // We cannot directly assign nullptr because of a check in assign():
    Var<EventBase> evbase = EventBase::assign<record_if_called>(
        (event_base *) 128, true);
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

static int fail(event_base *, evutil_socket_t, short, event_callback_fn,
                void *, const timeval *) { return -1; };

TEST_CASE("EventBase::once deals with event_base_once() failure") {
    Var<EventBase> evb = EventBase::create();
    REQUIRE_THROWS_AS(evb->once<fail>(0, EV_TIMEOUT, nullptr, nullptr),
                      EventBaseOnceError);
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

TEST_CASE("Bufferevent::socket_new deals with bufferevent_socket_new failure") {
    LibeventMock mock;
    mock.bufferevent_socket_new = [](event_base *, evutil_socket_t, int) {
        return (bufferevent *)nullptr;
    };
    REQUIRE_THROWS_AS(
        Bufferevent::socket_new(&mock, EventBase::create(), -1, 0),
        BuffereventSocketNewError);
}

// This function ensures that the Bufferevent allocated by `socket_new` is
// properly destroyed once done, by removing all the references to it
static void with_bufferevent(std::function<void(Var<Bufferevent>)> cb) {
    LibeventMock mock;
    Var<EventBase> evbase = EventBase::create();
    Var<Bufferevent> bufev = Bufferevent::socket_new(&mock, evbase, -1, 0);
    cb(bufev);
    delete bufev->the_opaque;
    bufev->the_opaque = nullptr;
    bufev = nullptr;
}

TEST_CASE("Bufferevent::socket_connect deals with bufferevent_socket_connect "
          "failure") {
    with_bufferevent([](Var<Bufferevent> bufev) {
        bufev->mockp->bufferevent_socket_connect =
            [](bufferevent *, sockaddr *, int) { return -1; };
        REQUIRE_THROWS_AS(
            Bufferevent::socket_connect(bufev->mockp, bufev, nullptr, 0),
            BuffereventSocketConnectError);
    });
}

TEST_CASE("Bufferevent::write deals with bufferevent_write failure") {
    with_bufferevent([](Var<Bufferevent> bufev) {
        bufev->mockp->bufferevent_write =
            [](bufferevent *, const void *, size_t) { return -1; };
        REQUIRE_THROWS_AS(Bufferevent::write(bufev->mockp, bufev, nullptr, 0),
                          BuffereventWriteError);
    });
}

TEST_CASE("Bufferevent::write_buffer deals with bufferevent_write_buffer "
          "failure") {
    with_bufferevent([](Var<Bufferevent> bufev) {
        bufev->mockp->bufferevent_write_buffer =
            [](bufferevent *, evbuffer *) { return -1; };
        Var<Evbuffer> evbuf = Evbuffer::create();
        REQUIRE_THROWS_AS(Bufferevent::write_buffer(bufev->mockp, bufev, evbuf),
                          BuffereventWriteBufferError);
    });
}

TEST_CASE("Bufferevent::write_buffer behaves on bufferevent_write_buffer "
          "success") {
    with_bufferevent([](Var<Bufferevent> bufev) {
        bufev->mockp->bufferevent_write_buffer =
            [](bufferevent *, evbuffer *) { return 0; };
        Var<Evbuffer> evbuf = Evbuffer::create();
        Bufferevent::write_buffer(bufev->mockp, bufev, evbuf);
    });
}

TEST_CASE("Bufferevent::read_buffer deals with bufferevent_read_buffer "
          "failure") {
    with_bufferevent([](Var<Bufferevent> bufev) {
        bufev->mockp->bufferevent_read_buffer =
            [](bufferevent *, evbuffer *) { return -1; };
        Var<Evbuffer> evbuf = Evbuffer::create();
        REQUIRE_THROWS_AS(Bufferevent::read_buffer(bufev->mockp, bufev, evbuf),
                          BuffereventReadBufferError);
    });
}

TEST_CASE("Bufferevent::enable deals with bufferevent_enable failure") {
    with_bufferevent([](Var<Bufferevent> bufev) {
        bufev->mockp->bufferevent_enable =
            [](bufferevent *, int) { return -1; };
        REQUIRE_THROWS_AS(Bufferevent::enable(bufev->mockp, bufev, EV_READ),
                          BuffereventEnableError);
    });
}

TEST_CASE("Bufferevent::enable deals with successful bufferevent_enable") {
    with_bufferevent([](Var<Bufferevent> bufev) {
        bufev->mockp->bufferevent_enable = [](bufferevent *, int) { return 0; };
        Bufferevent::enable(bufev->mockp, bufev, EV_READ);
    });
}

TEST_CASE("Bufferevent::disable deals with bufferevent_disable failure") {
    with_bufferevent([](Var<Bufferevent> bufev) {
        bufev->mockp->bufferevent_disable =
            [](bufferevent *, int) { return -1; };
        REQUIRE_THROWS_AS(Bufferevent::disable(bufev->mockp, bufev, EV_READ),
                          BuffereventDisableError);
    });
}

TEST_CASE("Bufferevent::disable deals with successful bufferevent_disable") {
    with_bufferevent([](Var<Bufferevent> bufev) {
        bufev->mockp->bufferevent_disable =
            [](bufferevent *, int) { return 0; };
        Bufferevent::disable(bufev->mockp, bufev, EV_READ);
    });
}

TEST_CASE("Bufferevent::set_timeouts deals with bufferevent_set_timeouts "
          "failure") {
    with_bufferevent([](Var<Bufferevent> bufev) {
        bufev->mockp->bufferevent_set_timeouts =
            [](bufferevent *, const timeval *, const timeval *) { return -1; };
        REQUIRE_THROWS_AS(
            Bufferevent::set_timeouts(bufev->mockp, bufev, nullptr, nullptr),
            BuffereventSetTimeoutsError);
    });
}

TEST_CASE("Bufferevent::openssl_filter_new deals with "
          "bufferevent_openssl_filter_new failure") {
    with_bufferevent([](Var<Bufferevent> bufev) {
        bufev->mockp->bufferevent_openssl_filter_new = [](
            event_base *, bufferevent *, ssl_st *, enum bufferevent_ssl_state,
            int) { return (bufferevent *)nullptr; };
        REQUIRE_THROWS_AS(
            Bufferevent::openssl_filter_new(bufev->mockp, bufev->evbase, bufev,
                                            nullptr, BUFFEREVENT_SSL_OPEN, 0),
            BuffereventOpensslFilterNewError);
    });
}

TEST_CASE("Bufferevent::get_input returns evbuffer not owning pointer") {
    with_bufferevent([](Var<Bufferevent> bufev) {
        Var<Evbuffer> evbuf = Bufferevent::get_input(bufev);
        REQUIRE(evbuf->owned == false);
    });
}

TEST_CASE("Bufferevent::get_output returns evbuffer not owning pointer") {
    with_bufferevent([](Var<Bufferevent> bufev) {
        Var<Evbuffer> evbuf = Bufferevent::get_output(bufev);
        REQUIRE(evbuf->owned == false);
    });
}
