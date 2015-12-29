// Part of measurement-kit <https://measurement-kit.github.io/>.
// Measurement-kit is free software. See AUTHORS and LICENSE for more
// information on the copying conditions.

#define MKOK_LIBEVENT_ENABLE_MOCK
#define CATCH_CONFIG_MAIN

#include <mkok/libevent.hpp>
#include "test/catch.hpp"

// Exception

TEST_CASE("Exception default constructor works") {
    Exception exc;
    REQUIRE(exc.file == "");
    REQUIRE(exc.line == 0);
    REQUIRE(exc.func == "");
    REQUIRE(exc.error == MKOK_LIBEVENT_GENERIC_ERROR);
}

TEST_CASE("Exception custom constructor works") {
    Exception exc("A", 17, "B", MKOK_LIBEVENT_LIBEVENT_ERROR);
    REQUIRE(exc.file == "A");
    REQUIRE(exc.line == 17);
    REQUIRE(exc.func == "B");
    REQUIRE(exc.error == MKOK_LIBEVENT_LIBEVENT_ERROR);
}

TEST_CASE("NoException constructor works") {
    NoException exc("A", 17, "B");
    REQUIRE(exc.file == "A");
    REQUIRE(exc.line == 17);
    REQUIRE(exc.func == "B");
    REQUIRE(exc.error == MKOK_LIBEVENT_NO_ERROR);
}

TEST_CASE("GenericException constructor works") {
    GenericException exc("A", 17, "B");
    REQUIRE(exc.file == "A");
    REQUIRE(exc.line == 17);
    REQUIRE(exc.func == "B");
    REQUIRE(exc.error == MKOK_LIBEVENT_GENERIC_ERROR);
}

TEST_CASE("NullPointerException constructor works") {
    NullPointerException exc("A", 17, "B");
    REQUIRE(exc.file == "A");
    REQUIRE(exc.line == 17);
    REQUIRE(exc.func == "B");
    REQUIRE(exc.error == MKOK_LIBEVENT_NULL_POINTER_ERROR);
}

TEST_CASE("LibeventException constructor works") {
    LibeventException exc("A", 17, "B");
    REQUIRE(exc.file == "A");
    REQUIRE(exc.line == 17);
    REQUIRE(exc.func == "B");
    REQUIRE(exc.error == MKOK_LIBEVENT_LIBEVENT_ERROR);
}

// Var

class TestingVar {
  public:
    int field = 42;
};

TEST_CASE("Var::get() provides no-null-pointer-returned guarantee") {
    Var<void> pointer;
    REQUIRE_THROWS_AS(pointer.get(), NullPointerException);
}

TEST_CASE("Var::get() returns the pointee") {
    Var<int> pointer(new int{42});
    REQUIRE(*(pointer.get()) == 42);
}

TEST_CASE("Var::operator->() provides no-null-pointer-returned guarantee") {
    Var<void> pointer;
    REQUIRE_THROWS_AS(pointer.operator->(), NullPointerException);
}

TEST_CASE("Var::operator->() returns the pointee") {
    Var<TestingVar> pointer(new TestingVar);
    REQUIRE(pointer->field == 42);
}

// Func

TEST_CASE("Func::Func() creates an empty function") {
    Func<void()> func;
    REQUIRE(static_cast<bool>(func) == false);
}

TEST_CASE("Func::Func(func) works as expected") {
    bool called = false;
    Func<void()> func([&called]() { called = true; });
    REQUIRE(static_cast<bool>(func) == true);
    func();
    REQUIRE(called == true);
}

TEST_CASE("We can assign a std::function to an existing Func") {
    bool called = false;
    Func<void()> func;
    func = [&called]() { called = true; };
    func();
    REQUIRE(called == true);
}

TEST_CASE("We can assign nullptr to an existing Func") {
    Func<void()> func;
    func = nullptr;
}

// Evutil

TEST_CASE("We deal with evutil_make_socket_nonblocking success") {
    Mock mock;
    mock.evutil_make_socket_nonblocking = [](evutil_socket_t) { return 0; };
    Evutil::make_socket_nonblocking(&mock, 0);
}

TEST_CASE("We deal with evutil_make_socket_nonblocking failure") {
    Mock mock;
    mock.evutil_make_socket_nonblocking = [](evutil_socket_t) { return -1; };
    REQUIRE_THROWS_AS(Evutil::make_socket_nonblocking(&mock, 0),
                      LibeventException);
}

TEST_CASE("We deal with evutil_parse_sockaddr_port success") {
    Mock mock;
    mock.evutil_parse_sockaddr_port = [](const char *, sockaddr *, int *) {
        return 0;
    };
    Evutil::parse_sockaddr_port(&mock, "", nullptr, nullptr);
}

TEST_CASE("We deal with evutil_parse_sockaddr_port failure") {
    Mock mock;
    mock.evutil_parse_sockaddr_port = [](const char *, sockaddr *, int *) {
        return -1;
    };
    REQUIRE_THROWS_AS(Evutil::parse_sockaddr_port(&mock, "", nullptr, nullptr),
                      LibeventException);
}

TEST_CASE("We deal with evutil_make_listen_socket_reuseable success") {
    Mock mock;
    mock.evutil_make_listen_socket_reuseable = [](evutil_socket_t) {
        return 0;
    };
    Evutil::make_listen_socket_reuseable(&mock, 0);
}

TEST_CASE("We deal with evutil_make_listen_socket_reuseable failure") {
    Mock mock;
    mock.evutil_make_listen_socket_reuseable = [](evutil_socket_t) {
        return -1;
    };
    REQUIRE_THROWS_AS(Evutil::make_listen_socket_reuseable(&mock, 0),
                      LibeventException);
}

// EventBase

TEST_CASE("EventBase default constructor works") {
    EventBase evbase;
    REQUIRE(evbase.evbase == nullptr);
    REQUIRE(evbase.owned == false);
    REQUIRE(evbase.mockp == nullptr);
}

TEST_CASE("EventBase does not call destructor if owned is false") {
    EventBase evbase;
    evbase.owned = false;
    Mock mock;
    mock.event_base_free = [](event_base *) { throw std::exception(); };
    evbase.mockp = &mock;
}

TEST_CASE("EventBase does not call destructor if owned is false and evbase is "
          "nullptr") {
    EventBase evbase;
    evbase.owned = true;
    Mock mock;
    mock.event_base_free = [](event_base *) { throw std::exception(); };
    evbase.mockp = &mock;
}

TEST_CASE("EventBase calls destructor if owned is true") {
    bool was_called = false;
    {
        EventBase evbase;
        evbase.owned = true;
        evbase.evbase = (event_base *)0xdeadbeef;
        Mock mock;
        mock.event_base_free = [&was_called](event_base *) {
            was_called = true;
        };
        evbase.mockp = &mock;
    }
    REQUIRE(was_called);
}

TEST_CASE("EventBase::assign throws if passed a nullptr") {
    Mock mock;
    REQUIRE_THROWS_AS(EventBase::assign(&mock, nullptr, true),
                      NullPointerException); 
}

TEST_CASE("EventBase::create deals with event_base_new() failure") {
    Mock mock;
    mock.event_base_new = []() -> event_base * { return nullptr; };
    REQUIRE_THROWS_AS(EventBase::create(&mock), NullPointerException);
}

TEST_CASE("EventBase::assign correctly sets mock") {
    bool called = false;
    Mock mock;
    mock.event_base_free = [&called](event_base *) { called = true; };
    Var<EventBase> evb = EventBase::assign(&mock, (event_base *)17, true);
    evb = nullptr;
    REQUIRE(called == true);
}

TEST_CASE("EventBase::dispatch deals with event_base_dispatch() returning 0") {
    Mock mock;
    mock.event_base_dispatch = [](event_base *) { return 0; };
    Var<EventBase> evb = EventBase::create(&mock);
    EventBase::dispatch(&mock, evb);
}

TEST_CASE("EventBase::dispatch deals with event_base_dispatch() returning 1") {
    Mock mock;
    mock.event_base_dispatch = [](event_base *) { return 1; };
    Var<EventBase> evb = EventBase::create(&mock);
    EventBase::dispatch(&mock, evb);
}

TEST_CASE("EventBase::dispatch deals with event_base_dispatch() failure") {
    Mock mock;
    mock.event_base_dispatch = [](event_base *) { return -1; };
    Var<EventBase> evb = EventBase::create(&mock);
    REQUIRE_THROWS_AS(EventBase::dispatch(&mock, evb), LibeventException);
}

TEST_CASE("EventBase::loop deals with event_base_loop() returning 0") {
    Mock mock;
    mock.event_base_loop = [](event_base *, int) { return 0; };
    Var<EventBase> evb = EventBase::create(&mock);
    EventBase::loop(&mock, evb, 0);
}

TEST_CASE("EventBase::loop deals with event_base_loop() returning 1") {
    Mock mock;
    mock.event_base_loop = [](event_base *, int) { return 1; };
    Var<EventBase> evb = EventBase::create(&mock);
    EventBase::loop(&mock, evb, 0);
}

TEST_CASE("EventBase::loop deals with event_base_loop() failure") {
    Mock mock;
    mock.event_base_loop = [](event_base *, int) { return -1; };
    Var<EventBase> evb = EventBase::create(&mock);
    REQUIRE_THROWS_AS(EventBase::loop(&mock, evb, 0), LibeventException);
}

TEST_CASE("EventBase::loopbreak deals with event_base_loopbreak() failure") {
    Mock mock;
    mock.event_base_loopbreak = [](event_base *) { return -1; };
    Var<EventBase> evb = EventBase::create(&mock);
    REQUIRE_THROWS_AS(EventBase::loopbreak(&mock, evb), LibeventException);
}

TEST_CASE("EventBase::once deals with event_base_once() failure") {
    Mock mock;
    mock.event_base_once = [](event_base *, evutil_socket_t, short,
                              event_callback_fn, void *, const timeval *) {
                                  return -1;
                              };
    Var<EventBase> evb = EventBase::create(&mock);
    REQUIRE_THROWS_AS(EventBase::once(&mock, evb, 0, EV_TIMEOUT, nullptr,
                                      nullptr), LibeventException);
}

// Evbuffer

TEST_CASE("Evbuffer::assign deals with nullptr input") {
    Mock mock;
    REQUIRE_THROWS_AS(Evbuffer::assign(&mock, nullptr, true),
                      NullPointerException);
}

TEST_CASE("Evbuffer::assign correctly sets mock") {
    bool called = false;
    Mock mock;
    mock.evbuffer_free = [&called](evbuffer *) { called = true; };
    Var<Evbuffer> evb = Evbuffer::assign(&mock, (evbuffer *)17, true);
    evb = nullptr;
    REQUIRE(called == true);
}

TEST_CASE("Evbuffer::pullup deals with evbuffer_pullup failure") {
    Mock mock;
    mock.evbuffer_pullup = [](evbuffer *, ssize_t) {
        return (unsigned char *) nullptr;
    };
    Var<Evbuffer> evb = Evbuffer::create(&mock);
    REQUIRE_THROWS_AS(Evbuffer::pullup(&mock, evb, -1), LibeventException);
}

// Bufferevent

TEST_CASE("Bufferevent::socket_new deals with bufferevent_socket_new failure") {
    Mock mock;
    mock.bufferevent_socket_new = [](event_base *, evutil_socket_t, int) {
        return (bufferevent *) nullptr;
    };
    REQUIRE_THROWS_AS(Bufferevent::socket_new(&mock, EventBase::create(&mock),
                                              -1, 0), LibeventException);
}

TEST_CASE("Bufferevent::socket_connect deals with bufferevent_socket_connect "
          "failure") {
    Mock mock;
    mock.bufferevent_socket_connect = [](bufferevent *, sockaddr *, int) {
        return -1;
    };
    Var<EventBase> evbase = EventBase::create(&mock);
    Var<Bufferevent> bufev = Bufferevent::socket_new(&mock, evbase, -1, 0);
    REQUIRE_THROWS_AS(Bufferevent::socket_connect(&mock, bufev, nullptr, 0),
                      LibeventException);
}

TEST_CASE("Bufferevent::write deals with bufferevent_write failure") {
    Mock mock;
    mock.bufferevent_write = [](bufferevent *, const void *, size_t) {
        return -1;
    };
    Var<EventBase> evbase = EventBase::create(&mock);
    Var<Bufferevent> bufev = Bufferevent::socket_new(&mock, evbase, -1, 0);
    REQUIRE_THROWS_AS(Bufferevent::write(&mock, bufev, nullptr, 0),
                      LibeventException);
}

TEST_CASE("Bufferevent::write_buffer deals with bufferevent_write_buffer "
          "failure") {
    Mock mock;
    mock.bufferevent_write_buffer = [](bufferevent *, evbuffer *) {
        return -1;
    };
    Var<EventBase> evbase = EventBase::create(&mock);
    Var<Bufferevent> bufev = Bufferevent::socket_new(&mock, evbase, -1, 0);
    Var<Evbuffer> evbuf = Evbuffer::create(&mock);
    REQUIRE_THROWS_AS(Bufferevent::write_buffer(&mock, bufev, evbuf),
                      LibeventException);
}

TEST_CASE("Bufferevent::read_buffer deals with bufferevent_read_buffer "
          "failure") {
    Mock mock;
    mock.bufferevent_read_buffer = [](bufferevent *, evbuffer *) {
        return -1;
    };
    Var<EventBase> evbase = EventBase::create(&mock);
    Var<Bufferevent> bufev = Bufferevent::socket_new(&mock, evbase, -1, 0);
    Var<Evbuffer> evbuf = Evbuffer::create(&mock);
    REQUIRE_THROWS_AS(Bufferevent::read_buffer(&mock, bufev, evbuf),
                      LibeventException);
}

TEST_CASE("Bufferevent::enable deals with bufferevent_enable failure") {
    Mock mock;
    mock.bufferevent_enable = [](bufferevent *, int) {
        return -1;
    };
    Var<EventBase> evbase = EventBase::create(&mock);
    Var<Bufferevent> bufev = Bufferevent::socket_new(&mock, evbase, -1, 0);
    REQUIRE_THROWS_AS(Bufferevent::enable(&mock, bufev, EV_READ),
                      LibeventException);
}

TEST_CASE("Bufferevent::enable deals with successful bufferevent_enable") {
    Mock mock;
    mock.bufferevent_enable = [](bufferevent *, int) {
        return 0;
    };
    Var<EventBase> evbase = EventBase::create(&mock);
    Var<Bufferevent> bufev = Bufferevent::socket_new(&mock, evbase, -1, 0);
    Bufferevent::enable(&mock, bufev, EV_READ);
}

TEST_CASE("Bufferevent::disable deals with bufferevent_disable failure") {
    Mock mock;
    mock.bufferevent_disable = [](bufferevent *, int) {
        return -1;
    };
    Var<EventBase> evbase = EventBase::create(&mock);
    Var<Bufferevent> bufev = Bufferevent::socket_new(&mock, evbase, -1, 0);
    REQUIRE_THROWS_AS(Bufferevent::disable(&mock, bufev, EV_READ),
                      LibeventException);
}

TEST_CASE("Bufferevent::disable deals with successful bufferevent_disable") {
    Mock mock;
    mock.bufferevent_disable = [](bufferevent *, int) {
        return 0;
    };
    Var<EventBase> evbase = EventBase::create(&mock);
    Var<Bufferevent> bufev = Bufferevent::socket_new(&mock, evbase, -1, 0);
    Bufferevent::disable(&mock, bufev, EV_READ);
}

TEST_CASE("Bufferevent::set_timeouts deals with bufferevent_set_timeouts "
          "failure") {
    Mock mock;
    mock.bufferevent_set_timeouts = [](bufferevent *, const timeval *,
                                       const timeval *) {
                                           return -1;
                                       };
    Var<EventBase> evbase = EventBase::create(&mock);
    Var<Bufferevent> bufev = Bufferevent::socket_new(&mock, evbase, -1, 0);
    REQUIRE_THROWS_AS(Bufferevent::set_timeouts(&mock, bufev, nullptr, nullptr),
                      LibeventException);
}

TEST_CASE("Bufferevent::openssl_filter_new deals with "
          "bufferevent_openssl_filter_new failure") {
    Mock mock;
    mock.bufferevent_openssl_filter_new = [](event_base *, bufferevent *,
                                             SSL *, enum bufferevent_ssl_state,
                                             int) {
                                                 return (bufferevent *) nullptr;
                                             };
    Var<EventBase> evbase = EventBase::create(&mock);
    Var<Bufferevent> bufev = Bufferevent::socket_new(&mock, evbase, -1, 0);
    REQUIRE_THROWS_AS(Bufferevent::openssl_filter_new(&mock, evbase,
                                                      bufev, nullptr,
                                                      BUFFEREVENT_SSL_OPEN,
                                                      0),
                      LibeventException);
}

TEST_CASE("Evbuffer destructor not called for Evbuffer returned by "
          "Bufferevent::get_input") {
    bool called = false;
    Mock mock;
    mock.evbuffer_free = [&called](evbuffer *) { called = true; };
    Var<EventBase> evbase = EventBase::create(&mock);
    Var<Bufferevent> bufev = Bufferevent::socket_new(&mock, evbase, -1, 0);
    Var<Evbuffer> evbuf = Bufferevent::get_input(&mock, bufev);
    evbuf = nullptr;
    REQUIRE(called == false);
}

TEST_CASE("Evbuffer destructor not called for Evbuffer returned by "
          "Bufferevent::get_output") {
    bool called = false;
    Mock mock;
    mock.evbuffer_free = [&called](evbuffer *) { called = true; };
    Var<EventBase> evbase = EventBase::create(&mock);
    Var<Bufferevent> bufev = Bufferevent::socket_new(&mock, evbase, -1, 0);
    Var<Evbuffer> evbuf = Bufferevent::get_output(&mock, bufev);
    evbuf = nullptr;
    REQUIRE(called == false);
}
