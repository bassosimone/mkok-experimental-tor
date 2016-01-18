// Part of measurement-kit <https://measurement-kit.github.io/>.
// Measurement-kit is free software. See AUTHORS and LICENSE for more
// information on the copying conditions.

#ifndef SRC_NET_SOCKS_HPP
#define SRC_NET_SOCKS_HPP

#include <cstddef>
#include <event2/util.h>
#include <exception>
#include <functional>
#include <measurement_kit/common/error.hpp>
#include <measurement_kit/common/func.hpp>
#include <measurement_kit/common/var.hpp>
#include <memory>
#include <stdlib.h>
#include <string>
#include <utility>
#include "src/common/libevent-core.hpp"
#include "src/common/libevent-evbuffer.hpp"
#include "src/common/libevent-evutil.hpp"

namespace mk {

/// \addtogroup errors
/// \{

/// Statuses returned by the Socks::connect() call
enum class SocksStatus {
    OK,                    ///< No error
    CONNECT_FAILED,        ///< Failed to connect
    UNEXPECTED_VERSION,    ///< Unexpected SOCKS version
    PROTO_ERROR,           ///< SOCKS protocol error
    ADDRESS_TOO_LONG,      ///< The passed address is too long
    INVALID_ATYPE,         ///< Invalid address type
    IO_ERROR_STEP_2,       ///< I/O error during step 2
    IO_ERROR_STEP_4,       ///< I/O error during step 4
    INVALID_PROXY_ADDRESS, ///< Passed invalid proxy address
    INVALID_PORT,          ///< Passed invalid port
};

/// \}
/// \addtogroup regress
/// \{

#ifdef SRC_NET_SOCKS_ENABLE_MOCK

class SocksMock {
  public:
#define XX(name, signature) std::function<signature> name = Bufferevent::name
    class {
      public:
        XX(socket_new, Var<Bufferevent>(Var<EventBase>, evutil_socket_t, int));
        XX(setcb, void(Var<Bufferevent>, std::function<void()>,
                       std::function<void()>, std::function<void(short)>));
        XX(get_output, Var<Evbuffer>(Var<Bufferevent>));
        XX(get_input, Var<Evbuffer>(Var<Bufferevent>));
        XX(set_timeouts,
           void(Var<Bufferevent>, const timeval *, const timeval *));
        XX(socket_connect, void(Var<Bufferevent>, sockaddr *, int));
        XX(enable, void(Var<Bufferevent>, int));
    } bufferevent;
#undef XX

#define XX(name, signature) std::function<signature> name = Evutil::name
    class {
      public:
        XX(parse_sockaddr_port, void(std::string, sockaddr *, int *));
    } evutil;
#undef XX

#define XX(name, signature) std::function<signature> name = Evbuffer::name
    class {
      public:
        XX(add_uint8, void(Var<Evbuffer>, uint8_t));
        XX(get_length, size_t(Var<Evbuffer>));
        XX(remove, std::string(Var<Evbuffer>, size_t));
        XX(add, void(Var<Evbuffer>, const void *, size_t));
        XX(add_uint16, void(Var <Evbuffer>, uint16_t));
        XX(copyout, std::string(Var<Evbuffer>, size_t));
        XX(drain, void(Var<Evbuffer>, size_t));
    } evbuffer;
#undef XX
};

#define MockPtrArg SocksMock *mockp,
#define MockPtrArg0 SocksMock *mockp
#define MockPtrName mockp,

#define Bufferevent(func, ...) mockp->bufferevent.func(__VA_ARGS__)
#define Evutil(func, ...) mockp->evutil.func(__VA_ARGS__)
#define Evbuffer(func, ...) mockp->evbuffer.func(__VA_ARGS__)

#else

#define MockPtrArg
#define MockPtrArg0
#define MockPtrName

#define Bufferevent(func, ...) Bufferevent::func(__VA_ARGS__)
#define Evutil(func, ...) evutil::func(__VA_ARGS__)
#define Evbuffer(func, ...) Evbuffer::func(__VA_ARGS__)

#endif

/// \}
/// \addtogroup wrappers
/// \{

/// Callback called to signal completion of connect() through proxy
typedef std::function<void(SocksStatus, Var<Bufferevent>)> SocksConnectCb;

/// Groups functions to connect to SOCKS proxy. In general the code in here
/// is fully asynchronous and does not throw exceptions unless something that
/// could not be recovered happens. The result of the connect() call is a
/// Var<Bufferevent> such that you can write generic networking code using
/// that type, regardless of whether it is connected through a proxy.
class Socks {
  public:
    /// Connect to remote address and port using SOCKS proxy.
    /// \param evbase Event base.
    /// \param host Remote address.
    /// \param port Remote port.
    /// \param cb Callback to notify completion.
    /// \param proxy_address Proxy address.
    /// \param proxy_port Proxy port.
    /// \param timeout Optional timeout (could be null).
    static void connect(MockPtrArg Var<EventBase> evbase, std::string host,
                        std::string port, SocksConnectCb cb,
                        std::string proxy_address = "127.0.0.1",
                        std::string proxy_port = "9050",
                        const timeval *timeout = nullptr) {
        connect(MockPtrName evbase, host, port, cb,
                proxy_address + ":" + proxy_port, timeout);
    }

    /// Connect to remote address and port using SOCKS proxy.
    /// \param evbase Event base.
    /// \param host Remote address.
    /// \param port Remote port.
    /// \param cb Callback to notify completion.
    /// \param proxy_endpoint Proxy endpoint.
    /// \param timeout Optional timeout (could be null).
    static void connect(MockPtrArg Var<EventBase> evbase, std::string host,
                        std::string port, SocksConnectCb cb,
                        std::string proxy_endpoint = "127.0.0.1:9050",
                        const timeval *timeout = nullptr) {
        if (port.length() > 5) {
            cb(SocksStatus::INVALID_PORT, nullptr);
            return;
        }
        for (char c : port) {
            if (!isdigit(c)) {
                cb(SocksStatus::INVALID_PORT, nullptr);
                return;
            }
        }
        uint16_t p = atoi(port.c_str());
        connect(MockPtrName evbase, host, p, cb, proxy_endpoint, timeout);
    }

    /// Connect to remote address and port using SOCKS proxy.
    /// \param evbase Event base.
    /// \param host Remote address.
    /// \param port Remote port.
    /// \param cb Callback to notify completion.
    /// \param proxy_endpoint Proxy endpoint.
    /// \param timeout Optional timeout (could be null).
    static void connect(MockPtrArg Var<EventBase> evbase, std::string host,
                        uint16_t port, SocksConnectCb cb,
                        std::string proxy_endpoint = "127.0.0.1:9050",
                        const timeval *timeout = nullptr) {
        sockaddr_storage storage;
        sockaddr *sa = (sockaddr *)&storage;
        int len = sizeof(storage);
        if (Evutil(parse_sockaddr_port, proxy_endpoint, sa, &len) != 0) {
            cb(SocksStatus::INVALID_PROXY_ADDRESS, nullptr);
            return;
        }
        connect(MockPtrName evbase, host, port, sa, len, cb, timeout);
    }

    static void connect(MockPtrArg Var<EventBase> evbase, std::string host,
                        uint16_t port, sockaddr *proxy_sa, int proxy_salen,
                        SocksConnectCb cb, const timeval *timeout = nullptr) {
        static const int flags = BEV_OPT_CLOSE_ON_FREE;
        Var<Bufferevent> bev = Bufferevent(socket_new, evbase, -1, flags);
        Bufferevent(
            setcb, bev, nullptr, nullptr,
            [ MockPtrName bev, cb, host, port ](short what) {
                if (what != BEV_EVENT_CONNECTED) {
                    // Remove self reference
                    Bufferevent(setcb, bev, nullptr, nullptr, nullptr);
                    cb(SocksStatus::CONNECT_FAILED, nullptr);
                    return;
                }
                Bufferevent(enable, bev, EV_READ);

                // Step #1: send out preferred authentication methods

                Var<Evbuffer> out = Bufferevent(get_output, bev);

                Evbuffer(add_uint8, out, 5); // Version
                Evbuffer(add_uint8, out, 1); // Number of methods
                Evbuffer(add_uint8, out, 0); // "NO_AUTH" meth.

                // Step #2: receive the allowed authentication methods

                Bufferevent(setcb, bev, [ MockPtrName bev, cb, host, port ]() {
                    Var<Evbuffer> in = Bufferevent(get_input, bev);
                    if (Evbuffer(get_length, in) < 2) {
                        return; // Try again after next recv()
                    }
                    std::string s = Evbuffer(remove, in, 2);
                    if (s[0] != 5) {
                        // Remove self reference
                        Bufferevent(setcb, bev, nullptr, nullptr, nullptr);
                        cb(SocksStatus::UNEXPECTED_VERSION, nullptr);
                        return;
                    }
                    if (s[1] != 0) {
                        // Remove self reference
                        Bufferevent(setcb, bev, nullptr, nullptr, nullptr);
                        cb(SocksStatus::PROTO_ERROR, nullptr);
                        return;
                    }

                    // Step #3: ask Tor to connect to remote host

                    Var<Evbuffer> out = Bufferevent(get_output, bev);

                    Evbuffer(add_uint8, out, 5); // Version
                    Evbuffer(add_uint8, out, 1); // CMD_CONNECT
                    Evbuffer(add_uint8, out, 0); // Reserved
                    Evbuffer(add_uint8, out, 3); // ATYPE_DOMAINNAME

                    if (host.length() > 255) {
                        // Remove self reference
                        Bufferevent(setcb, bev, nullptr, nullptr, nullptr);
                        cb(SocksStatus::ADDRESS_TOO_LONG, nullptr);
                        return;
                    }

                    // Length and host name
                    Evbuffer(add_uint8, out, host.length());
                    Evbuffer(add, out, host.c_str(), host.length());

                    // Port
                    Evbuffer(add_uint16, out, port);

                    // Step #4: receive Tor's response

                    Bufferevent(setcb, bev, [ MockPtrName bev, cb ]() {
                        Var<Evbuffer> in = Bufferevent(get_input, bev);
                        if (Evbuffer(get_length, in) < 5) {
                            return; // Try again after next recv()
                        }
                        std::string s = Evbuffer(copyout, in, 5);

                        // Version | Reply | Reserved
                        if (s[0] != 5 || s[1] != 0 || s[2] != 0) {
                            // Remove self reference
                            Bufferevent(setcb, bev, nullptr, nullptr, nullptr);
                            // TODO: Here we should process s[1] more
                            // carefully to map to the error that
                            // occurred and report it to the caller
                            cb(SocksStatus::PROTO_ERROR, nullptr);
                            return;
                        }

                        char atype = s[3]; // Atype

                        size_t total = 4; // Version .. Atype size
                        if (atype == 1) {
                            total += 4; // IPv4 addr size
                        } else if (atype == 3) {
                            // Len size + string size
                            total += 1 + s[4];
                        } else if (atype == 4) {
                            total += 16; // IPv6 addr size
                        } else {
                            // Remove self reference
                            Bufferevent(setcb, bev, nullptr, nullptr, nullptr);
                            cb(SocksStatus::INVALID_ATYPE, nullptr);
                            return;
                        }
                        total += 2; // Port size
                        if (Evbuffer(get_length, in) < total) {
                            return; // Try again after next recv()
                        }

                        // Remove socks data and clear callbacks
                        Evbuffer(drain, in, total);
                        Bufferevent(setcb, bev, nullptr, nullptr, nullptr);

                        // Step #5: success! callback!

                        cb(SocksStatus::OK, bev);
                    },
                    nullptr, [MockPtrName bev, cb](short) {
                        // Remove self reference
                        Bufferevent(setcb, bev, nullptr, nullptr, nullptr);
                        cb(SocksStatus::IO_ERROR_STEP_4, nullptr);
                    });

                },
                nullptr, [MockPtrName bev, cb](short) {
                    // Remove self reference
                    Bufferevent(setcb, bev, nullptr, nullptr, nullptr);
                    cb(SocksStatus::IO_ERROR_STEP_2, nullptr);
                });
            });

        Bufferevent(set_timeouts, bev, timeout, timeout);
        Bufferevent(socket_connect, bev, proxy_sa, proxy_salen);
    }
};

// Undefine internal macros
#undef MockPtrArg
#undef MockPtrArg0
#undef MockPtrName
#undef Bufferevent
#undef Evutil
#undef Evbuffer

/// \}

} // namespace mk
#endif
