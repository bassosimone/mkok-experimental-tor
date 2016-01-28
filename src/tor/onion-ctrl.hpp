// Part of measurement-kit <https://measurement-kit.github.io/>.
// Measurement-kit is free software. See AUTHORS and LICENSE for more
// information on the copying conditions.

#ifndef SRC_TOR_ONION_CTRL_HPP
#define SRC_TOR_ONION_CTRL_HPP

#include <ctype.h>
#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <event2/event.h>
#include <fstream>
#include <functional>
#include <iomanip>
#include <memory>
#include <sstream>
#include <stdlib.h>
#include <string>
#include <sys/socket.h>
#include <type_traits>
#include <vector>
#include "src/common/libevent.hpp"

// Forward declarations
struct timeval;

namespace mk {

/// \addtogroup errors
/// \{

/// Result of command sent to tor. Network and parse errors are negative,
/// while protocol errors are positive and correspond to the error codes that
/// are actually returned by tor (i.e. OK is 250).
enum class OnionStatus {
    ASYNC = 650,                        ///< Async reply.
    UNRECOGNIZED_ENTITY = 552,          ///< Unrecognized entity.
    OK = 250,                           ///< Request succeeded.
    NO_ERROR = 0,                       ///< No error.
    GENERIC_ERROR = -1,                 ///< Generic error.
    CONNECT_FAILED = -2,                ///< Connect failed.
    UNEXPECTED_REPLY = -3,              ///< Unexpected reply received.
    EXPECTED_NOTICE_TOKEN = -4,         ///< NOTICE token was not found.
    EXPECTED_BOOTSTRAP_TOKEN = -5,      ///< BOOTSTRAP token not found.
    EXPECTED_PROGRESS_KEY = -6,         ///< PROGRESS= key not found.
    EXPECTED_MORE_TOKENS = -7,          ///< More tokens were expected.
    TOO_MANY_DIGITS = -8,               ///< Integer contained too many digits.
    NOT_A_DIGIT = -9,                   ///< Non-digit character in integer.
    EXPECTED_VAR_NAME_KEY = -10,        ///< Expected variable name key.
    EXPECTED_STATUS_CLIENT_TOKEN = -11, ///< Expected STATUS_CLIENT token.
    UNEXPECTED_STATUS = -12,            ///< Unexpected status received.
    TIMEOUT_ERROR = -13,                ///< Timeout error.
    IO_ERROR = -14,                     ///< I/O error.
    EOF_ERROR = -15,                    ///< Unexpected EOF error.
    UNKNOWN_STATUS_ERROR = -16,         ///< Received an unknown status.
    ALREADY_CONNECTED = -17,            ///< We're already connected.
    CANNOT_PARSE_ADDRPORT = -18,        ///< Cannot parse addrport string.
};

/// \}
/// \addtogroup regress
/// \{

#ifdef SRC_TOR_ONION_CTRL_ENABLE_MOCK

class OnionCtrlMock {
  public:
    class {
      public:
        std::function<Error(std::string, sockaddr *, int *)>
            parse_sockaddr_port = [](std::string s, sockaddr *p, int *n) {
                return evutil::parse_sockaddr_port(s, p, n);
            };
    } evutil_impl;
};

#define MockPtrArg OnionCtrlMock *mockp,
#define MockPtrName mockp,
#define Evutil(x) mockp->evutil_impl.x

#else

#define MockPtrArg
#define MockPtrName
#define Evutil(x) evutil::x

#endif

/// \}
/// \addtogroup callbacks
/// \{

/// Callback to signal result of connect().
typedef std::function<void(OnionStatus)> OnionConnectCb;

/// Callback to signal result of command that replies with no data.
typedef std::function<void(OnionStatus)> OnionReplyVoidCb;

/// Callback to signal result of command that replies with integer value.
typedef std::function<void(OnionStatus, int)> OnionReplyIntCb;

/// Callback to signal result of command that replies with string value.
typedef std::function<void(OnionStatus, std::string)> OnionReplyStringCb;

/// Callback called to provide CLIENT_STATUS async notification.
typedef std::function<void(OnionStatus, std::string, std::string,
                           std::vector<std::string>)> OnionClientStatusCb;

/// Callback used internally to process line of reply.
typedef std::function<void(OnionStatus, char, std::string)> OnionReplyLineCb;

/// Callback called when the command was sent over the control connection.
typedef std::function<void(OnionStatus)> OnionSentCb;

/// \}

/// Code for interacting with tor. This code assumes that tor is already
/// running with an open control port. It allows you to connect to control
/// port, authenticate, send commands and receive replies.
///
/// For some specific commands there are high level functions that allow
/// you to issue the command and provide a callback to be called when data
/// is available again. For other commands, you can always use the low
/// level sendrecv() function that sends a request as a string and receives
/// the corresponding reply line (or lines) as string.
///
/// The connection with the control port is represented by a Var<OnionCtrl>
/// object, where Var is a shared-ptr-like smart pointer.
///
/// Here and in mkok-libevent-ng we encourage a programming style by which
/// objects, such as Var<OnionCtrl>, are kept alive by storing references
/// to them into their own callbacks. This creates reference cycles that
/// guarantee safe usage but leak memory unless the callbacks of an object
/// are cleared when the object is no longer needed. This is why you need
/// to call `OnionCtrl::close(ctrl);` onto the Var<OnionCtrl> returned
/// by this library when you don't need it anymore.
///
/// See the example folder for actual examples of usage.
class OnionCtrl {
  public:
    /// \addtogroup setup
    /// \{

    Var<EventBase> evbase; ///< Event base
    Var<Bufferevent> bev;  ///< Bufferevent

    /// Create control connection.
    /// \param evbase Event base variable.
    /// \return New control connection.
    static Var<OnionCtrl> create(Var<EventBase> evbase) {
        Var<OnionCtrl> ctrl(new OnionCtrl);
        ctrl->evbase = evbase;
        return ctrl;
    }

    /// Close control connection.
    /// \param ctrl Control connection.
    static void close(MockPtrArg Var<OnionCtrl> ctrl) {
        // Remove self reference
        ctrl->bev->setcb(nullptr, nullptr, nullptr);
    }

    /// Connect to control port and authenticate.
    /// \param ctrl Onion controller.
    /// \param cb Callback called to signal completion.
    /// \param auth_token String containing authentication information.
    /// \param port Port to use (default is 9051).
    /// \param address Address to use (default is "127.0.0.1").
    /// \param timeout Timeout for connection (default is no timeout).
    static void connect_and_authenticate(MockPtrArg Var<OnionCtrl> ctrl,
                                         OnionConnectCb cb,
                                         std::string auth_token = "",
                                         int port = 9051,
                                         std::string address = "127.0.0.1",
                                         const timeval *timeout = nullptr) {
        std::stringstream epnt;
        epnt << address << ":" << port;
        connect_and_authenticate(MockPtrName ctrl, epnt.str(), cb, auth_token,
                                 timeout);
    }

    /// Connect to control port and authenticate.
    /// \param ctrl Onion controller.
    /// \param epnt Endpoint (i.e. ADDRESS":"PORT) to connect to.
    /// \param cb Callback called to signal completion.
    /// \param auth_token String containing authentication information.
    /// \param timeout Timeout for connection (default is no timeout).
    static void connect_and_authenticate(MockPtrArg Var<OnionCtrl> ctrl,
                                         std::string epnt, OnionConnectCb cb,
                                         std::string auth_token = "",
                                         const timeval *timeout = nullptr) {
        sockaddr_storage storage;
        sockaddr *sa = (sockaddr *)&storage;
        int len = sizeof(storage);
        if (Evutil(parse_sockaddr_port)(epnt, sa, &len) != 0) {
            cb(OnionStatus::CANNOT_PARSE_ADDRPORT);
            return;
        }
        connect_and_authenticate(MockPtrName ctrl, sa, len, cb, auth_token,
                                 timeout);
    }

    /// Connect to control port and authenticate.
    /// \param ctrl Onion controller.
    /// \param sa Pointer to sockaddr containing destination address.
    /// \param len Length of data pointed by sa.
    /// \param cb Callback called to signal completion.
    /// \param auth_token String containing authentication information.
    /// \param timeout Timeout for connection (default is no timeout).
    static void connect_and_authenticate(MockPtrArg Var<OnionCtrl> ctrl,
                                         sockaddr *sa, int len,
                                         OnionConnectCb cb,
                                         std::string auth_token = "",
                                         const timeval *timeout = nullptr) {
        connect(MockPtrName ctrl, sa, len,
                [ MockPtrName auth_token, ctrl, cb ](OnionStatus status) {
                    if (status != OnionStatus::NO_ERROR) {
                        cb(status);
                        return;
                    }
                    sendrecv(MockPtrName ctrl, cmd_authenticate(auth_token),
                             [cb](OnionStatus status, char type, std::string) {
                                 if (status != OnionStatus::OK) {
                                     cb(status);
                                     return;
                                 }
                                 if (type != ' ') {
                                     cb(OnionStatus::UNEXPECTED_REPLY);
                                     return;
                                 }
                                 cb(status);
                             });
                },
                timeout);
    }

    /// Connect to the control connection.
    /// \param ctrl Onion controller.
    /// \param sa Pointer to sockaddr containing destination address.
    /// \param len Length of data pointed by sa.
    /// \param cb Callback called to signal completion.
    /// \param timeout Timeout for connection (default is no timeout).
    static void connect(MockPtrArg Var<OnionCtrl> ctrl, sockaddr *sa, int len,
                        OnionConnectCb cb, const timeval *timeout = nullptr) {
        static const int flags = BEV_OPT_CLOSE_ON_FREE;
        if (ctrl->bev) {
            cb(OnionStatus::ALREADY_CONNECTED);
            return;
        }
        ctrl->bev = Bufferevent::socket_new(ctrl->evbase, -1, flags);
        ctrl->bev->setcb(
            nullptr, nullptr, [ MockPtrName ctrl, cb ](short what) {
                if (what != BEV_EVENT_CONNECTED) {
                    // Remove self reference
                    ctrl->bev->setcb(nullptr, nullptr, nullptr);
                    cb(OnionStatus::CONNECT_FAILED);
                    return;
                }
                cb(OnionStatus::NO_ERROR);
            });
        ctrl->bev->set_timeouts(timeout, timeout);
        ctrl->bev->socket_connect(sa, len);
    }

    /// Generate AUTHENTICATE command.
    /// \param param Parameter for AUTHENTICATE command (default is empty).
    /// \return String to be sent over control connection.
    static std::string cmd_authenticate(std::string param = "") {
        std::string cmd = "AUTHENTICATE";
        if (param != "") {
            cmd += " ";
            cmd += param;
        }
        cmd += "\r\n";
        return cmd;
    }

    /// Reads auth-cookie from file and converts to hex.
    /// \param source Source file path.
    /// \return Content of file as hexadecimal string (success)
    ///         or empty string (failure).
    static std::string read_auth_cookie_as_hex(std::string source) {
        return auth_cookie_to_hex(read_auth_cookie_as_string(source));
    }

    /// Reads auth-cookie from file and return a string.
    /// \param source Source file path.
    /// \return Content of file (success) or empty string (failure).
    static std::string read_auth_cookie_as_string(std::string source) {
        std::ifstream file;
        file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        try {
            file.open(source);
            std::stringstream buffer;
            buffer << file.rdbuf();
            return buffer.str();
        } catch (...) {
            return "";
        }
    }

    /// Converts auth-cookie to hexadecimal string
    /// \param source Auth-cookie string.
    /// \return String encoded as hexadecimal.
    static std::string auth_cookie_to_hex(std::string source) {
        std::stringstream ss;
        ss << std::hex << std::setfill('0');
        for (size_t i = 0; i < source.length(); ++i) {
            ss << std::setw(2) << static_cast<unsigned int>(
                                      static_cast<unsigned char>(source[i]));
        }
        return ss.str();
    }

    /// \}
    /// \addtogroup getinfo
    /// \{

    /// Execute GETINFO status/bootstrap-phase and return result as integer.
    /// \param ctrl Control connection.
    /// \param cb Callback called to report reply status as integer.
    static void
    getinfo_status_bootstrap_phase_as_int(MockPtrArg Var<OnionCtrl> ctrl,
                                          OnionReplyIntCb cb) {
        getinfo_status_bootstrap_phase_as_string(
            MockPtrName ctrl, [cb](OnionStatus status, std::string s) {
                if (status != OnionStatus::OK) {
                    cb(status, 0);
                    return;
                }
                int phase = 0;
                status = parse_bootstrap_progress(s, phase);
                cb(status, phase);
            });
    }

    /// Execute GETINFO status/bootstrap-phase and return result as string.
    /// \param ctrl Control connection.
    /// \param cb Callback called to report reply status as string.
    static void
    getinfo_status_bootstrap_phase_as_string(MockPtrArg Var<OnionCtrl> ctrl,
                                             OnionReplyStringCb cb) {
        Var<std::string> result(new std::string);
        sendrecv(MockPtrName ctrl, "GETINFO status/bootstrap-phase\r\n",
                 [cb, result](OnionStatus status, char type, std::string s) {
                     if (status != OnionStatus::OK) {
                         cb(status, "");
                         return;
                     }
                     if (type == '-' &&
                         s.find("status/bootstrap-phase=") == 0) {
                         s = s.substr(sizeof("status/bootstrap-phase=") - 1);
                         *result = s;
                         return;
                     }
                     if (type != ' ') {
                         cb(OnionStatus::UNEXPECTED_REPLY, "");
                         return;
                     }
                     cb(status, *result);
                 });
    }

    /// Parse string returned by GETINFO status/bootstrap-phase.
    /// \param s String returned to us by tor.
    /// \param phase Phase of the boostrap (between 0 and 100).
    /// \return OnionStatus::OK on success, an error status otherwise.
    static OnionStatus parse_bootstrap_progress(std::string s, int &phase) {
        // E.g. NOTICE BOOTSTRAP PROGRESS=0 TAG=starting SUMMARY="Starting"
        std::vector<std::string> vec = tokenize(s);
        if (vec.size() < 3) {
            return OnionStatus::EXPECTED_MORE_TOKENS;
        }
        if (vec[0] != "NOTICE") {
            return OnionStatus::EXPECTED_NOTICE_TOKEN;
        }
        if (vec[1] != "BOOTSTRAP") {
            return OnionStatus::EXPECTED_BOOTSTRAP_TOKEN;
        }
        if (vec[2].find("PROGRESS=") != 0) {
            return OnionStatus::EXPECTED_PROGRESS_KEY;
        }
        vec[2] = vec[2].substr(sizeof("PROGRESS=") - 1);
        if (vec[2].size() > 3) {
            return OnionStatus::TOO_MANY_DIGITS;
        }
        for (char &c : vec[2]) {
            if (!isdigit(c)) {
                return OnionStatus::NOT_A_DIGIT;
            }
        }
        phase = atoi(vec[2].c_str());
        return OnionStatus::OK;
    }

    /// \}
    /// \addtogroup getconf
    /// \{

    /// Send GETCONF SOCKSPort command and process response.
    /// \param ctrl Control connection.
    /// \param cb Callback called when the response is received.
    static void getconf_socks_port(MockPtrArg Var<OnionCtrl> ctrl,
                                   OnionReplyIntCb cb) {
        sendrecv(MockPtrName ctrl, "GETCONF SOCKSPort\r\n",
                 [cb](OnionStatus status, char type, std::string s) {
                     getconf_int(status, type, s, "SocksPort", 9050, cb);
                 });
    }

    /// Set GETCONF DisableNetwork command and process response.
    /// \param ctrl Control connection.
    /// \param cb Callback called when the response is received.
    static void getconf_disable_network(MockPtrArg Var<OnionCtrl> ctrl,
                                        OnionReplyIntCb cb) {
        sendrecv(MockPtrName ctrl, "GETCONF DisableNetwork\r\n",
                 [cb](OnionStatus status, char type, std::string s) {
                     getconf_int(status, type, s, "DisableNetwork", 0, cb);
                 });
    }

    /// Helper function to read integer configuration variable.
    /// \param status Status as returned by sendrecv().
    /// \param type Type as returned by sendrecv().
    /// \param s String as returned by sendrecv().
    /// \param var_name Name of variable to be read.
    /// \param default_value Default value to use for variable.
    /// \param cb Callback called on success.
    static void getconf_int(OnionStatus status, char type, std::string s,
                            std::string var_name, int default_value,
                            OnionReplyIntCb cb) {
        if (status != OnionStatus::OK) {
            cb(status, 0);
            return;
        }
        if (type != ' ') {
            cb(OnionStatus::UNEXPECTED_REPLY, 0);
            return;
        }
        if (s == var_name) {
            cb(OnionStatus::OK, default_value); // = default value
            return;
        }
        if (s.find(var_name) != 0) {
            cb(OnionStatus::EXPECTED_VAR_NAME_KEY, 0);
            return;
        }
        s = s.substr(var_name.size() + 1);
        if (s.size() > 5) {
            cb(OnionStatus::TOO_MANY_DIGITS, 0);
            return;
        }
        for (char &c : s) {
            if (!isdigit(c)) {
                cb(OnionStatus::NOT_A_DIGIT, 0);
                return;
            }
        }
        cb(OnionStatus::OK, atoi(s.c_str()));
    }

    /// \}
    /// \addtogroup signal
    /// \{

    /// Send SIGNAL SHUTDOWN command and receive response.
    /// \param ctrl Control connecction.
    /// \param cb Callback called when tor replies.
    static void signal_shutdown(MockPtrArg Var<OnionCtrl> ctrl,
                                OnionReplyVoidCb cb) {
        sendrecv(MockPtrName ctrl, "SIGNAL SHUTDOWN\r\n",
                 [cb](OnionStatus status, char type, std::string) {
                     do_void_cb(status, type, cb);
                 });
    }

    /// \}
    /// \addtogroup setconf
    /// \{

    /// Call SETCONF DisableNetwork=[0|1] command and read response.
    /// \param ctrl Control connection.
    /// \param value value Whether to disable or enable network.
    /// \param cb Callback called on reply.
    static void setconf_disable_network(MockPtrArg Var<OnionCtrl> ctrl,
                                        bool value, OnionReplyVoidCb cb) {
        sendrecv(MockPtrName ctrl, cmd_disable_network(value),
                 [cb](OnionStatus status, char type, std::string) {
                     do_void_cb(status, type, cb);
                 });
    }

    /// Generates line for SETCONF DisableNetwork command.
    /// \param value Bool value to indicate whether to enable or disable.
    /// \return The string to send to tor.
    static std::string cmd_disable_network(bool value) {
        std::string cmd = "SETCONF DisableNetwork=";
        if (value) {
            cmd += "1";
        } else {
            cmd += "0";
        }
        cmd += "\r\n";
        return cmd;
    }

    /// \}
    /// \addtogroup setevents
    /// \{

    /// Send the SETEVENTS CLIENT_STATUS command and receive async replies.
    /// \param ctrl Control connection.
    /// \param cb Callback receiving client status notification.
    static void setevents_client_status(MockPtrArg Var<OnionCtrl> ctrl,
                                        OnionClientStatusCb cb) {
        setevents_client_status_as_string(
            MockPtrName ctrl, [cb](OnionStatus status, std::string s) {
                if (status != OnionStatus::ASYNC) {
                    cb(status, "", "", {});
                    return;
                }
                std::vector<std::string> vec = tokenize(s);
                if (vec.size() < 3) {
                    cb(OnionStatus::EXPECTED_MORE_TOKENS, "", "", {});
                    return;
                }
                if (vec[0] != "STATUS_CLIENT") {
                    cb(OnionStatus::EXPECTED_STATUS_CLIENT_TOKEN, "", "", {});
                    return;
                }
                std::string severity = vec[1];
                std::string action = vec[2];
                vec = std::vector<std::string>(vec.begin() + 3, vec.end());
                cb(status, severity, action, vec);
            });
    }

    /// Send the SETEVENTS CLIENT_STATUS command and receive async replies.
    /// \param ctrl Control connection.
    /// \param cb Callback receiving client status notification.
    static void
    setevents_client_status_as_string(MockPtrArg Var<OnionCtrl> ctrl,
                                      OnionReplyStringCb cb) {
        Var<OnionStatus> expected(new OnionStatus(OnionStatus::OK));
        sendrecv(MockPtrName ctrl, "SETEVENTS STATUS_CLIENT\r\n",
                 [cb, expected](OnionStatus status, char type, std::string s) {
                     if (status != *expected) {
                         cb(OnionStatus::UNEXPECTED_STATUS, "");
                         return;
                     }
                     if (type != ' ') {
                         cb(OnionStatus::UNEXPECTED_REPLY, "");
                         return;
                     }
                     *expected = OnionStatus::ASYNC;
                     if (status == OnionStatus::OK) {
                         return;
                     }
                     cb(status, s);
                 });
    }

    /// \}
    /// \addtogroup util
    /// \{

    /// Common code to process void reply to command.
    /// \param status OnionStatus as returned by sendrecv().
    /// \param type type (one of ' ', '+', '-') as returned by sendrecv().
    /// \param cb Callback to pass the result to.
    static void do_void_cb(OnionStatus status, char type, OnionReplyVoidCb cb) {
        if (status != OnionStatus::OK) {
            cb(status);
            return;
        }
        if (type != ' ') {
            cb(OnionStatus::UNEXPECTED_REPLY);
            return;
        }
        cb(status);
    }

    /// Send a command and receive reply.
    /// \param ctrl Control connection.
    /// \param command Command to send to control port.
    /// \param cb Callback called on reply.
    static void sendrecv(MockPtrArg Var<OnionCtrl> ctrl, std::string command,
                         OnionReplyLineCb cb) {
        send(MockPtrName ctrl, command,
             [ MockPtrName ctrl, cb ](OnionStatus status) {
                 if (status != OnionStatus::NO_ERROR) {
                     cb(status, 0, "");
                     return;
                 }
                 recv(MockPtrName ctrl, cb);
             });
    }

    /// Tokenize string s.
    /// \param s String to tokenize.
    /// \return Tokens.
    static std::vector<std::string> tokenize(std::string s) {
        std::vector<std::string> vector;
        std::string cur;
        for (char &c : s) {
            if (isspace(c)) {
                if (cur != "") {
                    vector.push_back(cur);
                    cur = "";
                }
            } else {
                cur += c;
            }
        }
        if (cur != "") vector.push_back(cur);
        return vector;
    }

    /// Send command to control port.
    /// \param ctrl Control connection.
    /// \param command Command to send to control port.
    /// \param cb Callback called when data was sent.
    static void send(MockPtrArg Var<OnionCtrl> ctrl, std::string command,
                     OnionSentCb cb) {
        ctrl->bev->write(command.data(), command.size());
        ctrl->bev->setcb(nullptr,
                           [cb]() { cb(OnionStatus::NO_ERROR); },
                           [cb](short w) { cb(event_mask_to_status(w)); });
    }

    /// Receive lines from control port.
    /// \param ctrl Control connection.
    /// \param cb Callback that receives lines.
    static void recv(MockPtrArg Var<OnionCtrl> ctrl, OnionReplyLineCb cb) {
        ctrl->bev->enable(EV_READ); // Just in case, enable read
        ctrl->bev->setcb([ MockPtrName ctrl, cb ]() {
            for (;;) {
                Var<Evbuffer> input = ctrl->bev->get_input();
                std::string line = input->readln(EVBUFFER_EOL_CRLF);
                if (line == "") {
                    // "There are explicitly no limits on line length"
                    return;
                }
                if (line.size() < 4) {
                    cb(OnionStatus::EXPECTED_MORE_TOKENS, 0, "");
                    return;
                }
                if (!isdigit(line[0]) || !isdigit(line[1]) ||
                    !isdigit(line[2])) {
                    cb(OnionStatus::NOT_A_DIGIT, 0, "");
                    return;
                }
                char type = line[3];
                int result = atoi(line.substr(0, 3).c_str());
                line = line.substr(4);
                if (result == 250) {
                    cb(OnionStatus::OK, type, line);
                } else if (result == 552) {
                    cb(OnionStatus::UNRECOGNIZED_ENTITY, type, line);
                } else if (result == 650) {
                    cb(OnionStatus::ASYNC, type, line);
                } else {
                    cb(OnionStatus::UNKNOWN_STATUS_ERROR, type, line);
                }
            }
        },
        nullptr,
        [cb](short w) {
            cb(event_mask_to_status(w), 0, "");
        });
    }

    /// Map event mask returned by libevent to status.
    static OnionStatus event_mask_to_status(short w) {
        if ((w & BEV_EVENT_TIMEOUT) != 0) {
            return OnionStatus::TIMEOUT_ERROR;
        } else if ((w & BEV_EVENT_ERROR) != 0) {
            return OnionStatus::IO_ERROR;
        } else if ((w & BEV_EVENT_EOF) != 0) {
            return OnionStatus::EOF_ERROR;
        } else {
            return OnionStatus::GENERIC_ERROR;
        }
    }
};

/// \}

#undef MockPtrArg
#undef MockPtrName
#undef Evutil

} // namespace

#endif
