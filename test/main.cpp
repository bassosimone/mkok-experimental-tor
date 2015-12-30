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
    REQUIRE(exc.error == Error::GENERIC);
}

TEST_CASE("The exception mechanism works as expected") {
#define XX(exc_name, err_name)                                                 \
    SECTION("" #exc_name " constructor works") {                               \
        exc_name exc("A", 17, "B");                                            \
        REQUIRE(exc.file == "A");                                              \
        REQUIRE(exc.line == 17);                                               \
        REQUIRE(exc.func == "B");                                              \
        REQUIRE(exc.error == Error::err_name);                                 \
    }                                                                          \
                                                                               \
    SECTION("Exception throwing works for " #exc_name "") {                    \
        try {                                                                  \
            MKOK_LIBEVENT_THROW(exc_name);                                     \
        } catch (Exception & exc) {                                            \
            REQUIRE(exc.file == __FILE__);                                     \
            REQUIRE(exc.line == __LINE__);                                     \
            REQUIRE(exc.func == __func__);                                     \
            REQUIRE(exc.error == Error::err_name);                             \
        }                                                                      \
    }
    XX(NoException, NO_ERROR);
    XX(GenericException, GENERIC);
    XX(NullPointerException, NULL_POINTER);
    XX(EvutilMakeSocketNonblockingException, EVUTIL_MAKE_SOCKET_NONBLOCKING);
    XX(EvutilParseSockaddrPortException, EVUTIL_PARSE_SOCKADDR_PORT);
    XX(EvutilMakeListenSocketReuseableException,
       EVUTIL_MAKE_LISTEN_SOCKET_REUSEABLE);
    XX(EventBaseDispatchException, EVENT_BASE_DISPATCH);
    XX(EventBaseLoopException, EVENT_BASE_LOOP);
    XX(EventBaseLoopbreakException, EVENT_BASE_LOOPBREAK);
    XX(EventBaseOnceException, EVENT_BASE_ONCE);
    XX(EvbufferAddException, EVBUFFER_ADD);
    XX(EvbufferAddBufferException, EVBUFFER_ADD_BUFFER);
    XX(EvbufferDrainException, EVBUFFER_DRAIN);
    XX(EvbufferPeekException, EVBUFFER_PEEK);
    XX(EvbufferPeekMismatchException, EVBUFFER_PEEK_MISMATCH);
    XX(EvbufferPullupException, EVBUFFER_PULLUP);
    XX(EvbufferRemoveBufferException, EVBUFFER_REMOVE_BUFFER);
    XX(BuffereventSocketNewException, BUFFEREVENT_SOCKET_NEW);
    XX(BuffereventSocketConnectException, BUFFEREVENT_SOCKET_CONNECT);
    XX(BuffereventWriteException, BUFFEREVENT_WRITE);
    XX(BuffereventWriteBufferException, BUFFEREVENT_WRITE_BUFFER);
    XX(BuffereventReadBufferException, BUFFEREVENT_READ_BUFFER);
    XX(BuffereventEnableException, BUFFEREVENT_ENABLE);
    XX(BuffereventDisableException, BUFFEREVENT_DISABLE);
    XX(BuffereventSetTimeoutsException, BUFFEREVENT_SET_TIMEOUTS);
    XX(BuffereventOpensslFilterNewException, BUFFEREVENT_OPENSSL_FILTER_NEW);
#undef XX
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
                      EvutilMakeSocketNonblockingException);
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
                      EvutilParseSockaddrPortException);
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
                      EvutilMakeListenSocketReuseableException);
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
    REQUIRE_THROWS_AS(EventBase::dispatch(&mock, evb),
                      EventBaseDispatchException);
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
    REQUIRE_THROWS_AS(EventBase::loop(&mock, evb, 0), EventBaseLoopException);
}

TEST_CASE("EventBase::loopbreak deals with event_base_loopbreak() failure") {
    Mock mock;
    mock.event_base_loopbreak = [](event_base *) { return -1; };
    Var<EventBase> evb = EventBase::create(&mock);
    REQUIRE_THROWS_AS(EventBase::loopbreak(&mock, evb),
                      EventBaseLoopbreakException);
}

TEST_CASE("EventBase::once deals with event_base_once() failure") {
    Mock mock;
    mock.event_base_once = [](event_base *, evutil_socket_t, short,
                              event_callback_fn, void *,
                              const timeval *) { return -1; };
    Var<EventBase> evb = EventBase::create(&mock);
    REQUIRE_THROWS_AS(
        EventBase::once(&mock, evb, 0, EV_TIMEOUT, nullptr, nullptr),
        EventBaseOnceException);
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
        return (unsigned char *)nullptr;
    };
    Var<Evbuffer> evb = Evbuffer::create(&mock);
    REQUIRE_THROWS_AS(Evbuffer::pullup(&mock, evb, -1),
                      EvbufferPullupException);
}

TEST_CASE("Evbuffer::drain deals with evbuffer_drain failure") {
    Mock mock;
    mock.evbuffer_drain = [](evbuffer *, size_t) { return -1; };
    Var<Evbuffer> evb = Evbuffer::create(&mock);
    REQUIRE_THROWS_AS(Evbuffer::drain(&mock, evb, 512), EvbufferDrainException);
}

TEST_CASE("Evbuffer::drain correctly deals with evbuffer_drain success") {
    Mock mock;
    mock.evbuffer_drain = [](evbuffer *, size_t) { return 0; };
    Var<Evbuffer> evb = Evbuffer::create(&mock);
    Evbuffer::drain(&mock, evb, 512);
}

TEST_CASE("Evbuffer::add deails with evbuffer_add failure") {
    Mock mock;
    mock.evbuffer_add = [](evbuffer *, const void *, size_t) { return -1; };
    Var<Evbuffer> evb = Evbuffer::create(&mock);
    REQUIRE_THROWS_AS(Evbuffer::add(&mock, evb, nullptr, 0),
                      EvbufferAddException);
}

TEST_CASE("Evbuffer::add correctly deails with evbuffer_add success") {
    Mock mock;
    mock.evbuffer_add = [](evbuffer *, const void *, size_t) { return 0; };
    Var<Evbuffer> evb = Evbuffer::create(&mock);
    Evbuffer::add(&mock, evb, nullptr, 0);
}

TEST_CASE("Evbuffer::add_buffer deails with evbuffer_add_buffer failure") {
    Mock mock;
    mock.evbuffer_add_buffer = [](evbuffer *, evbuffer *) { return -1; };
    Var<Evbuffer> evb = Evbuffer::create(&mock);
    Var<Evbuffer> b = Evbuffer::create(&mock);
    REQUIRE_THROWS_AS(Evbuffer::add_buffer(&mock, evb, b),
                      EvbufferAddBufferException);
}

TEST_CASE("Evbuffer::add_buffer correctly deails with evbuffer_add_buffer "
          "success") {
    Mock mock;
    mock.evbuffer_add_buffer = [](evbuffer *, evbuffer *) { return 0; };
    Var<Evbuffer> evb = Evbuffer::create(&mock);
    Var<Evbuffer> b = Evbuffer::create(&mock);
    Evbuffer::add_buffer(&mock, evb, b);
}

TEST_CASE("Evbuffer::peek deals with first evbuffer_peek()'s failure") {
    Mock mock;
    mock.evbuffer_peek =
        [](evbuffer *, ssize_t, evbuffer_ptr *, evbuffer_iovec *, int) {
            return -1;
        };
    Var<Evbuffer> evb = Evbuffer::create(&mock);
    size_t n_extents = 0;
    REQUIRE_THROWS_AS(Evbuffer::peek(&mock, evb, -1, nullptr, n_extents),
                      EvbufferPeekException);
}

TEST_CASE("Evbuffer::peek deals with evbuffer_peek() returning zero") {
    Mock mock;
    mock.evbuffer_peek =
        [](evbuffer *, ssize_t, evbuffer_ptr *, evbuffer_iovec *, int) {
            return 0;
        };
    Var<Evbuffer> evb = Evbuffer::create(&mock);
    size_t n_extents = 0;
    auto res = Evbuffer::peek(&mock, evb, -1, nullptr, n_extents);
    REQUIRE_THROWS_AS(res.get(), NullPointerException);
}

TEST_CASE("Evbuffer::peek deals with evbuffer_peek() mismatch") {
    auto count = 17;
    Mock mock;
    mock.evbuffer_peek =
        [&count](evbuffer *, ssize_t, evbuffer_ptr *, evbuffer_iovec *, int) {
            return count++;
        };
    Var<Evbuffer> evb = Evbuffer::create(&mock);
    size_t n_extents = 0;
    REQUIRE_THROWS_AS(Evbuffer::peek(&mock, evb, -1, nullptr, n_extents),
                      EvbufferPeekMismatchException);
}

static Var<Evbuffer> fill_with_n_extents(Mock *mock, int n) {
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
    Mock mock;
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
    Mock mock;
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
    Mock mock;
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
    Mock mock;
    Var<Evbuffer> evb = fill_with_n_extents(&mock, count);
    size_t num = 0;
    Evbuffer::for_each_(&mock, evb, [&num](const void *, size_t) {
        ++num;
        return false;
    });
    REQUIRE(num == 1);
}

TEST_CASE("Evbuffer::copyout works as expected for empty evbuffer") {
    Mock mock;
    Var<Evbuffer> evb = Evbuffer::create(&mock);
    REQUIRE(Evbuffer::get_length(evb) == 0);
    REQUIRE(Evbuffer::copyout(&mock, evb, 512) == "");
}

TEST_CASE("Evbuffer::copyout works as expected for evbuffer filled with data") {
    Mock mock;
    std::string source(4096, 'A');
    Var<Evbuffer> evb = Evbuffer::create(&mock);
    Evbuffer::add(&mock, evb, source.data(), source.size());
    std::string out = Evbuffer::copyout(&mock, evb, 1024);
    REQUIRE(out == std::string(1024, 'A'));
    REQUIRE(Evbuffer::get_length(evb) == 4096);
}

TEST_CASE("Evbuffer::remove works as expected for empty evbuffer") {
    Mock mock;
    Var<Evbuffer> evb = Evbuffer::create(&mock);
    REQUIRE(Evbuffer::get_length(evb) == 0);
    REQUIRE(Evbuffer::remove(&mock, evb, 512) == "");
}

TEST_CASE("Evbuffer::remove works as expected for evbuffer filled with data") {
    Mock mock;
    std::string source(4096, 'A');
    Var<Evbuffer> evb = Evbuffer::create(&mock);
    Evbuffer::add(&mock, evb, source.data(), source.size());
    std::string out = Evbuffer::remove(&mock, evb, 1024);
    REQUIRE(out == std::string(1024, 'A'));
    REQUIRE(Evbuffer::get_length(evb) == 3072);
}

TEST_CASE("Evbuffer::remove_buffer deals with evbuffer_remove_buffer failure") {
    Mock mock;
    mock.evbuffer_remove_buffer = [](evbuffer *, evbuffer *, size_t) {
        return -1;
    };
    Var<Evbuffer> evb = Evbuffer::create(&mock);
    Var<Evbuffer> b = Evbuffer::create(&mock);
    REQUIRE_THROWS_AS(Evbuffer::remove_buffer(&mock, evb, b, 512),
                      EvbufferRemoveBufferException);
}

static void test_evbuffer_remove_buffer_success(int retval) {
    Mock mock;
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
    Mock mock;
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
    Mock mock;
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
    Mock mock;
    mock.bufferevent_socket_new = [](event_base *, evutil_socket_t, int) {
        return (bufferevent *)nullptr;
    };
    REQUIRE_THROWS_AS(
        Bufferevent::socket_new(&mock, EventBase::create(&mock), -1, 0),
        BuffereventSocketNewException);
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
                      BuffereventSocketConnectException);
}

TEST_CASE("Bufferevent::write deals with bufferevent_write failure") {
    Mock mock;
    mock.bufferevent_write = [](bufferevent *, const void *, size_t) {
        return -1;
    };
    Var<EventBase> evbase = EventBase::create(&mock);
    Var<Bufferevent> bufev = Bufferevent::socket_new(&mock, evbase, -1, 0);
    REQUIRE_THROWS_AS(Bufferevent::write(&mock, bufev, nullptr, 0),
                      BuffereventWriteException);
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
                      BuffereventWriteBufferException);
}

TEST_CASE("Bufferevent::write_buffer behaves on bufferevent_write_buffer "
          "success") {
    Mock mock;
    mock.bufferevent_write_buffer = [](bufferevent *, evbuffer *) { return 0; };
    Var<EventBase> evbase = EventBase::create(&mock);
    Var<Bufferevent> bufev = Bufferevent::socket_new(&mock, evbase, -1, 0);
    Var<Evbuffer> evbuf = Evbuffer::create(&mock);
    Bufferevent::write_buffer(&mock, bufev, evbuf);
}

TEST_CASE("Bufferevent::read_buffer deals with bufferevent_read_buffer "
          "failure") {
    Mock mock;
    mock.bufferevent_read_buffer = [](bufferevent *, evbuffer *) { return -1; };
    Var<EventBase> evbase = EventBase::create(&mock);
    Var<Bufferevent> bufev = Bufferevent::socket_new(&mock, evbase, -1, 0);
    Var<Evbuffer> evbuf = Evbuffer::create(&mock);
    REQUIRE_THROWS_AS(Bufferevent::read_buffer(&mock, bufev, evbuf),
                      BuffereventReadBufferException);
}

TEST_CASE("Bufferevent::enable deals with bufferevent_enable failure") {
    Mock mock;
    mock.bufferevent_enable = [](bufferevent *, int) { return -1; };
    Var<EventBase> evbase = EventBase::create(&mock);
    Var<Bufferevent> bufev = Bufferevent::socket_new(&mock, evbase, -1, 0);
    REQUIRE_THROWS_AS(Bufferevent::enable(&mock, bufev, EV_READ),
                      BuffereventEnableException);
}

TEST_CASE("Bufferevent::enable deals with successful bufferevent_enable") {
    Mock mock;
    mock.bufferevent_enable = [](bufferevent *, int) { return 0; };
    Var<EventBase> evbase = EventBase::create(&mock);
    Var<Bufferevent> bufev = Bufferevent::socket_new(&mock, evbase, -1, 0);
    Bufferevent::enable(&mock, bufev, EV_READ);
}

TEST_CASE("Bufferevent::disable deals with bufferevent_disable failure") {
    Mock mock;
    mock.bufferevent_disable = [](bufferevent *, int) { return -1; };
    Var<EventBase> evbase = EventBase::create(&mock);
    Var<Bufferevent> bufev = Bufferevent::socket_new(&mock, evbase, -1, 0);
    REQUIRE_THROWS_AS(Bufferevent::disable(&mock, bufev, EV_READ),
                      BuffereventDisableException);
}

TEST_CASE("Bufferevent::disable deals with successful bufferevent_disable") {
    Mock mock;
    mock.bufferevent_disable = [](bufferevent *, int) { return 0; };
    Var<EventBase> evbase = EventBase::create(&mock);
    Var<Bufferevent> bufev = Bufferevent::socket_new(&mock, evbase, -1, 0);
    Bufferevent::disable(&mock, bufev, EV_READ);
}

TEST_CASE("Bufferevent::set_timeouts deals with bufferevent_set_timeouts "
          "failure") {
    Mock mock;
    mock.bufferevent_set_timeouts = [](bufferevent *, const timeval *,
                                       const timeval *) { return -1; };
    Var<EventBase> evbase = EventBase::create(&mock);
    Var<Bufferevent> bufev = Bufferevent::socket_new(&mock, evbase, -1, 0);
    REQUIRE_THROWS_AS(Bufferevent::set_timeouts(&mock, bufev, nullptr, nullptr),
                      BuffereventSetTimeoutsException);
}

TEST_CASE("Bufferevent::openssl_filter_new deals with "
          "bufferevent_openssl_filter_new failure") {
    Mock mock;
    mock.bufferevent_openssl_filter_new = [](event_base *, bufferevent *, SSL *,
                                             enum bufferevent_ssl_state, int) {
        return (bufferevent *)nullptr;
    };
    Var<EventBase> evbase = EventBase::create(&mock);
    Var<Bufferevent> bufev = Bufferevent::socket_new(&mock, evbase, -1, 0);
    REQUIRE_THROWS_AS(Bufferevent::openssl_filter_new(&mock, evbase, bufev,
                                                      nullptr,
                                                      BUFFEREVENT_SSL_OPEN, 0),
                      BuffereventOpensslFilterNewException);
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
