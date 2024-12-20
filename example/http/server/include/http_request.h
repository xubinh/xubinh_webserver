#ifndef __XUBINH_SERVER_HTTP_REQUEST
#define __XUBINH_SERVER_HTTP_REQUEST

#include <cstring>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>

#include "util/time_point.h"

namespace xubinh_server {

class HttpRequest {
public:
    enum HttpMethodType { UNSUPPORTED_HTTP_METHOD, GET, POST };

    enum HttpVersionType { UNSUPPORTED_HTTP_VERSION, HTTP_1_0, HTTP_1_1 };

    HttpRequest() = default;

    // no copy
    HttpRequest(const HttpRequest &) = delete;
    HttpRequest &operator=(const HttpRequest &) = delete;

    // no move
    HttpRequest(HttpRequest &&) = delete;
    HttpRequest &operator=(HttpRequest &&) = delete;

    ~HttpRequest() = default;

    bool parse_request_line(const char *start, const char *end);

    HttpMethodType get_method_type() const {
        return _method;
    }

    const char *get_method_type_as_string() const;

    const std::string &get_path() const {
        return _path;
    }

    HttpVersionType get_version_type() const {
        return _version;
    }

    const char *get_version_type_as_string() const;

    void set_receive_time_point(const util::TimePoint &receive_time_point) {
        _receive_time_point = receive_time_point;
    }

    const util::TimePoint &get_receive_time_point() const {
        return _receive_time_point;
    }

    // true = success, false = fail
    bool parse_header_line(const char *start, const char *end);

    const std::string &get_header(const std::string &key) const {
        auto it = _headers.find(key);

        if (it != _headers.end()) {
            return it->second;
        }

        return _empty_string;
    }

    bool need_close() const {
        return get_header("Connection") == "close";
    }

    // const std::unordered_map<std::string, std::string> &get_headers() const {
    const std::map<std::string, std::string> &get_headers() const {
        return _headers;
    }

    void set_body(const char *start, const char *end) {
        _body.assign(start, end);
    }

    const std::vector<char> &get_body() const {
        return _body;
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

    static const std::string _empty_string;

    HttpMethodType _method = UNSUPPORTED_HTTP_METHOD;
    std::string _path;
    HttpVersionType _version = UNSUPPORTED_HTTP_VERSION;
    util::TimePoint _receive_time_point;
    // std::unordered_map<std::string, std::string> _headers;
    std::map<std::string, std::string>
        _headers; // for testing; more efficient than std::map when the number
                  // of headers is small
    std::vector<char> _body;
};

} // namespace xubinh_server

#endif