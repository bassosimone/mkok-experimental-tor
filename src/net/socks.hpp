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
#define XX(name, signature) std::function<signature> name = Evutil::name
    class {
      public:
        XX(parse_sockaddr_port, void(std::string, sockaddr *, int *));
    } evutil;
#undef XX
};

#define MockPtrArg SocksMock *mockp,
#define MockPtrArg0 SocksMock *mockp
#define MockPtrName mockp,

#define Evutil(func, ...) mockp->evutil.func(__VA_ARGS__)

#else

#define MockPtrArg
#define MockPtrArg0
#define MockPtrName

#define Evutil(func, ...) evutil::func(__VA_ARGS__)

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
        Var<Bufferevent> bev = Bufferevent::socket_new(evbase, -1, flags);
        bev->setcb(
            nullptr, nullptr,
            [ MockPtrName bev, cb, host, port ](short what) {
                if (what != BEV_EVENT_CONNECTED) {
                    // Remove self reference
                    bev->setcb(nullptr, nullptr, nullptr);
                    cb(SocksStatus::CONNECT_FAILED, nullptr);
                    return;
                }
                bev->enable(EV_READ);

                // Step #1: send out preferred authentication methods

                Var<Evbuffer> out = bev->get_output();

                out->add_uint8(5); // Version
                out->add_uint8(1); // Number of methods
                out->add_uint8(0); // "NO_AUTH" meth.

                // Step #2: receive the allowed authentication methods

                bev->setcb([ MockPtrName bev, cb, host, port ]() {
                    Var<Evbuffer> in = bev->get_input();
                    if (in->get_length() < 2) {
                        return; // Try again after next recv()
                    }
                    std::string s = in->remove(2);
                    if (s[0] != 5) {
                        // Remove self reference
                        bev->setcb(nullptr, nullptr, nullptr);
                        cb(SocksStatus::UNEXPECTED_VERSION, nullptr);
                        return;
                    }
                    if (s[1] != 0) {
                        // Remove self reference
                        bev->setcb(nullptr, nullptr, nullptr);
                        cb(SocksStatus::PROTO_ERROR, nullptr);
                        return;
                    }

                    // Step #3: ask Tor to connect to remote host

                    Var<Evbuffer> out = bev->get_output();

                    out->add_uint8(5); // Version
                    out->add_uint8(1); // CMD_CONNECT
                    out->add_uint8(0); // Reserved
                    out->add_uint8(3); // ATYPE_DOMAINNAME

                    if (host.length() > 255) {
                        // Remove self reference
                        bev->setcb(nullptr, nullptr, nullptr);
                        cb(SocksStatus::ADDRESS_TOO_LONG, nullptr);
                        return;
                    }

                    // Length and host name
                    out->add_uint8(host.length());
                    out->add(host.c_str(), host.length());

                    // Port
                    out->add_uint16(port);

                    // Step #4: receive Tor's response

                    bev->setcb([ MockPtrName bev, cb ]() {
                        Var<Evbuffer> in = bev->get_input();
                        if (in->get_length() < 5) {
                            return; // Try again after next recv()
                        }
                        std::string s = in->copyout(5);

                        // Version | Reply | Reserved
                        if (s[0] != 5 || s[1] != 0 || s[2] != 0) {
                            // Remove self reference
                            bev->setcb(nullptr, nullptr, nullptr);
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
                            bev->setcb(nullptr, nullptr, nullptr);
                            cb(SocksStatus::INVALID_ATYPE, nullptr);
                            return;
                        }
                        total += 2; // Port size
                        if (in->get_length() < total) {
                            return; // Try again after next recv()
                        }

                        // Remove socks data and clear callbacks
                        in->drain(total);
                        bev->setcb(nullptr, nullptr, nullptr);

                        // Step #5: success! callback!

                        cb(SocksStatus::OK, bev);
                    },
                    nullptr, [MockPtrName bev, cb](short) {
                        // Remove self reference
                        bev->setcb(nullptr, nullptr, nullptr);
                        cb(SocksStatus::IO_ERROR_STEP_4, nullptr);
                    });

                },
                nullptr, [MockPtrName bev, cb](short) {
                    // Remove self reference
                    bev->setcb(nullptr, nullptr, nullptr);
                    cb(SocksStatus::IO_ERROR_STEP_2, nullptr);
                });
            });

        bev->set_timeouts(timeout, timeout);
        bev->socket_connect(proxy_sa, proxy_salen);
    }
};

// Undefine internal macros
#undef MockPtrArg
#undef MockPtrArg0
#undef MockPtrName
#undef Bufferevent
#undef Evutil

/// \}

} // namespace mk
#endif
