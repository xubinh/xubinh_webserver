#ifndef XUBINH_SERVER_HTTP_REQUEST
#define XUBINH_SERVER_HTTP_REQUEST

#include <cstring>
#include <string>
#include <unordered_map>

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

    // true = success, false = fail
    bool set_method_type(const char *start, const char *end);

    HttpMethodType get_method_type() const {
        return _method;
    }

    const char *get_method_type_as_string() const;

    void set_path(const char *start, const char *end) {
        _path.assign(start, end);
    }

    const std::string &get_path() const {
        return _path;
    }

    // true = success, false = fail
    bool set_version_type(const char *start, const char *end);

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
    bool set_header(const char *start, const char *end);

    const std::string &get_header(const std::string &key) const {
        auto it = _headers.find(key);

        if (it != _headers.end()) {
            return it->second;
        }

        return {};
    }

    const std::unordered_map<std::string, std::string> &get_headers() const {
        return _headers;
    }

    void set_body(const std::string &body) {
        _body = body;
    }

    void set_body(std::string &&body) {
        _body = std::move(body);
    }

    const std::string &get_body() const {
        return _body;
    }

private:
    HttpMethodType _method = UNSUPPORTED_HTTP_METHOD;
    std::string _path;
    HttpVersionType _version = UNSUPPORTED_HTTP_VERSION;
    util::TimePoint _receive_time_point;
    std::unordered_map<std::string, std::string> _headers;
    std::string _body;
};

} // namespace xubinh_server

#endif