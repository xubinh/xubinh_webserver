#ifndef __XUBINH_SERVER_HTTP_REQUEST
#define __XUBINH_SERVER_HTTP_REQUEST

#include <cstring>
#include <string>
#include <unordered_map>

#include "util/slab_allocator.h"
#include "util/time_point.h"

namespace xubinh_server {

class HttpRequest {
public:
    using StringType = util::StringType;

    enum HttpMethodType { UNSUPPORTED_HTTP_METHOD, GET, POST };

    enum HttpVersionType { UNSUPPORTED_HTTP_VERSION, HTTP_1_0, HTTP_1_1 };

    HttpRequest() = default;

    // dummy; abort if get called
    HttpRequest(const HttpRequest &);

    // dummy; abort if get called
    HttpRequest &operator=(const HttpRequest &);

    // implement this anyway just for sense of security
    HttpRequest(HttpRequest &&other)
        : _method(other._method), _path(std::move(other._path)),
          _version(other._version),
          _receive_time_point(other._receive_time_point),
          _headers(std::move(other._headers)), _body(std::move(other._body)) {
    }

    // implement this anyway just for sense of security
    HttpRequest &operator=(HttpRequest &&other) {
        if (this != &other) {
            _method = other._method;
            _path = std::move(other._path);
            _version = other._version;
            _receive_time_point = other._receive_time_point;
            _headers = std::move(other._headers);
            _body = std::move(other._body);
        }

        return *this;
    }

    bool parse_request_line(const char *start, const char *end);

    HttpMethodType get_method_type() const {
        return _method;
    }

    const char *get_method_type_as_string() const;

    const StringType &get_path() const {
        return _path;
    }

    HttpVersionType get_version_type() const {
        return _version;
    }

    const char *get_version_type_as_string() const;

    void set_receive_time_point(util::TimePoint receive_time_point) {
        _receive_time_point = receive_time_point;
    }

    util::TimePoint get_receive_time_point() const {
        return _receive_time_point;
    }

    // true = success, false = fail
    bool parse_header_line(const char *start, const char *end);

    const StringType &get_header(const StringType &key) const {
        auto it = _headers.find(key);

        if (it != _headers.end()) {
            return it->second;
        }

        return _empty_string;
    }

    // used by parser
    void set_need_close(bool need_close) {
        _need_close = need_close;
    }

    bool get_need_close() const {
        return _need_close;
    }

    const std::unordered_map<StringType, StringType> &get_headers() const {
        return _headers;
    }

    void set_body(const char *start, const char *end) {
        _body.assign(start, end);
    }

    const StringType &get_body() const {
        return _body;
    }

    void reset() noexcept {
        _method = UNSUPPORTED_HTTP_METHOD;
        _path.clear();
        _version = UNSUPPORTED_HTTP_VERSION;
        _receive_time_point = 0;
        _headers.clear();
        _body.clear();
    }

private:
    // HTTP header field names (keys) are case-insensitive, according to
    // HTTP/1.1 specification (RFC 7230, Section 3.2) which states:
    //
    // - "Field names are case-insensitive."
    static void to_lowercase(char *start, char *end) {
        auto it = start;

        while (it != end) {
            *it = ::tolower(*it);
        }
    }

    // true = success, false = fail
    bool _set_method_type(const char *start, const char *end);

    void _set_path(const char *start, const char *end) {
        _path.assign(start, end);
    }

    // true = success, false = fail
    bool _set_version_type(const char *start, const char *end);

    static const StringType _empty_string;

    HttpMethodType _method = UNSUPPORTED_HTTP_METHOD;
    StringType _path;
    HttpVersionType _version = UNSUPPORTED_HTTP_VERSION;
    util::TimePoint _receive_time_point{0};
    std::unordered_map<StringType, StringType> _headers;
    bool _need_close{false};
    StringType _body;
};

} // namespace xubinh_server

#endif