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
