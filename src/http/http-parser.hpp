// Part of measurement-kit <https://measurement-kit.github.io/>.
// Measurement-kit is free software. See AUTHORS and LICENSE for more
// information on the copying conditions.

#ifndef SRC_HTTP_PARSER_HPP
#define SRC_HTTP_PARSER_HPP

#include <cstddef>
#include <http_parser.h>
#include <measurement_kit/common/func.hpp>
#include <measurement_kit/common/var.hpp>
#include <measurement_kit/http/error.hpp>
#include <memory>

extern "C" {

#define EVENT_CB(name) static int mk_http_parser_##name(http_parser *p)

#define EVENT_DATA_CB(name)                                                    \
    static int mk_http_parser_##name(http_parser *p, const char *s, size_t n)

EVENT_CB(message_begin);     ///< C callback called on message-begin
EVENT_DATA_CB(status);       ///< C callback called on status
EVENT_DATA_CB(header_field); ///< C callback called on header-field
EVENT_DATA_CB(header_value); ///< C callback called on header-value
EVENT_CB(headers_complete);  ///< C callback called on headers-complete
EVENT_DATA_CB(body);         ///< C callback called on body
EVENT_CB(message_complete);  ///< C callback called on message-complete

// Undefine temporary macros
#undef EVENT_CB
#undef EVENT_DATA_CB

} // extern "C"

namespace mk {

/// Wrapper for nodejs/http-parser. In addition to the functionality provided
/// by such parser, it provides the following functionality:
///
/// 1) guarantee that, if the object is reachable, it is alive;
///
/// 2) guarantee that it is possible to keep the object alive by adding it
///    to the closure list of a lambda (including a lambda assigned to a
///    Func<T> member of the object itself;
///
/// 3) guarantee that, if there is an error, an exception is thrown.
class HttpParser {
  public:
    http_parser parser;            ///< The parser itself
    http_parser_settings settings; ///< Parser settings

#define EVENT_CB(name) Func<void()> cb_##name;
#define EVENT_DATA_CB(name) Func<void(const char *, size_t)> cb_##name;
    EVENT_CB(message_begin);     ///< C++ callback called on message-begin
    EVENT_DATA_CB(status);       ///< C++ callback called on status
    EVENT_DATA_CB(header_field); ///< C++ callback called on header-field
    EVENT_DATA_CB(header_value); ///< C++ callback called on header-value
    EVENT_CB(headers_complete);  ///< C++ callback called on headers-complete
    EVENT_DATA_CB(body);         ///< C++ callback called on body
    EVENT_CB(message_complete);  ///< C++ callback called on message-complete
#undef EVENT_CB
#undef EVENT_DATA_CB

    HttpParser() {}                                ///< Default ctor.
    HttpParser(HttpParser &) = delete;             ///< Deleted copy ctor.
    HttpParser &operator=(HttpParser &) = delete;  ///< Deleted copy assign.
    HttpParser(HttpParser &&) = delete;            ///< Deleted move ctor.
    HttpParser &operator=(HttpParser &&) = delete; ///< Deleted move assign.
    ~HttpParser() {}                               ///< Destructor.

    /// Creates http-parser object.
    /// \param type One of HTTP_REQUEST, HTTP_RESPONSE, HTTP_BOTH.
    /// \return New http-parser object.
    static Var<HttpParser> create(enum http_parser_type type) {
        Var<HttpParser> parser = std::make_shared<HttpParser>();
        http_parser_settings_init(&parser->settings);
#define XX(name) parser->settings.on_##name = mk_http_parser_##name
        XX(message_begin);
        XX(status);
        XX(header_field);
        XX(header_value);
        XX(headers_complete);
        XX(body);
        XX(message_complete);
#undef XX
        http_parser_init(&parser->parser, type);
        parser->parser.data = parser.get();
        return parser;
    }

    /// Parse data.
    /// \param parser Http parser.
    /// \param data First byte of data to parse (could be `nullptr`).
    /// \param count Number of bytes to parse (could be 0).
    /// \throw HttpParserUpgradeError.
    /// \throw HttpParserGenericParseError.
    static void parse(Var<HttpParser> parser, const char *data, size_t count) {
        size_t n = http_parser_execute(&parser->parser, &parser->settings, data,
                                       count);
        if (parser->parser.upgrade) {
            MK_THROW(HttpParserUpgradeError);
        }
        if (n != count) {
            MK_THROW(HttpParserGenericParseError);
        }
    }

    /// Tell the parser that we hit EOF.
    /// \param parser Http parser.
    /// \throw HttpParserUpgradeError.
    /// \throw HttpParserGenericParseError.
    static void eof(Var<HttpParser> parser) { parse(parser, nullptr, 0); }

    /// Remove all self references. This enables automatic destruction of this
    /// object, when it was saved inside one or more of its callbacks.
    /// \param parser Http parser.
    static void clear(Var<HttpParser> parser) {
        parser->cb_message_begin = nullptr;
        parser->cb_status = nullptr;
        parser->cb_header_field = nullptr;
        parser->cb_header_value = nullptr;
        parser->cb_headers_complete = nullptr;
        parser->cb_body = nullptr;
        parser->cb_message_complete = nullptr;
    }
};

} // namespace

extern "C" {

#define EVENT_CB(name)                                                         \
    static int mk_http_parser_##name(http_parser *p) {                         \
        auto parser = static_cast<mk::HttpParser *>(p->data);                  \
        if (parser->cb_##name) parser->cb_##name();                            \
        return 0;                                                              \
    }

#define EVENT_DATA_CB(name)                                                    \
    static int mk_http_parser_##name(http_parser *p, const char *s, size_t n) {\
        auto parser = static_cast<mk::HttpParser *>(p->data);                  \
        if (parser->cb_##name) parser->cb_##name(s, n);                        \
        return 0;                                                              \
    }

EVENT_CB(message_begin)
EVENT_DATA_CB(status)
EVENT_DATA_CB(header_field)
EVENT_DATA_CB(header_value)
EVENT_CB(headers_complete)
EVENT_DATA_CB(body)
EVENT_CB(message_complete)

// Undefine temporary macros
#undef XX
#undef EVENT_CB
#undef EVENT_DATA_CB

} // extern "C"

#endif
