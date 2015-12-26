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

// Evutil

TEST_CASE("We deal with evutil_make_socket_nonblocking failure") {
    Mock mock;
    mock.evutil_make_socket_nonblocking = [](evutil_socket_t) { return -1; };
    REQUIRE_THROWS_AS(Evutil::make_socket_nonblocking(&mock, 0),
                      LibeventException);
}

TEST_CASE("We deal with evutil_parse_sockaddr_port failure") {
    Mock mock;
    mock.evutil_parse_sockaddr_port = [](const char *, sockaddr *, int *) {
        return -1;
    };
    REQUIRE_THROWS_AS(Evutil::parse_sockaddr_port(&mock, "", nullptr, nullptr),
                      LibeventException);
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

TEST_CASE("EventBase::assign throws is passed a nullptr") {
    Mock mock;
    REQUIRE_THROWS_AS(EventBase::assign(&mock, nullptr, true),
                      NullPointerException); 
}

TEST_CASE("EventBase::create deals with event_base_new() failure") {
    Mock mock;
    mock.event_base_new = []() -> event_base * { return nullptr; };
    REQUIRE_THROWS_AS(EventBase::create(&mock), NullPointerException);
}
