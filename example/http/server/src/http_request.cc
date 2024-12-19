#include "../include/http_request.h"

namespace xubinh_server {

bool HttpRequest::parse_request_line(const char *start, const char *end) {
    auto method_start = start;

    // trim off leading spaces
    while (method_start < end && (*method_start) == ' ') {
        method_start++;
    }

    // the whole string was just spaces
    if (method_start == end) {
        return false;
    }

    auto method_end =
        static_cast<const char *>(::memchr(method_start, ' ', end - start));

    // no space, no method field
    if (method_end == nullptr) {
        return false;
    }

    if (!_set_method_type(method_start, method_end)) {
        return false;
    }

    // ---------

    auto path_start = method_end;

    // trim off leading spaces
    while (path_start < end && (*path_start) == ' ') {
        path_start++;
    }

    // the remaining string was just spaces
    if (path_start == end) {
        return false;
    }

    auto path_end =
        static_cast<const char *>(::memchr(path_start, ' ', end - start));

    // no space, no path field
    if (path_end == nullptr) {
        return false;
    }

    // [NOTE]: checking is done later by the application
    _set_path(path_start, path_end);

    // ---------

    auto version_start = path_end;

    // trim off leading spaces
    while (version_start < end && (*version_start) == ' ') {
        version_start++;
    }

    // the remaining string was just spaces
    if (version_start == end) {
        return false;
    }

    auto version_end = end;

    // trim off trailing spaces
    while (*(version_end - 1) == ' ') {
        version_end--;
    }

    return _set_version_type(version_start, version_end);
}

const char *HttpRequest::get_method_type_as_string() const {
    switch (_method) {
    case GET:
        return "GET";
        break;

    case POST:
        return "POST";
        break;

    default:
        return nullptr;
        break;
    }
}

const char *HttpRequest::get_version_type_as_string() const {
    switch (_version) {
    case HTTP_1_0:
        return "HTTP/1.0";
        break;

    case HTTP_1_1:
        return "HTTP/1.1";
        break;

    default:
        return nullptr;
        break;
    }
}

bool HttpRequest::parse_header_line(const char *start, const char *end) {
    const char *colon =
        reinterpret_cast<const char *>(::memchr(start, ':', end - start));

    if (colon == nullptr) {
        return false;
    }

    const char *key_start = start;
    const char *key_end = colon;

    while (key_start < key_end && ::isspace(*key_start)) {
        key_start++;
    }

    if (key_start == key_end) {
        return false;
    }

    while (::isspace(*(key_end - 1))) {
        key_end--;
    }

    // HttpRequest::to_lowercase(
    //     const_cast<char *>(key_start), const_cast<char *>(key_end)
    // );

    const char *value_start = colon + 1;
    const char *value_end = end;

    while (value_start < value_end && ::isspace(*value_start)) {
        value_start++;
    }

    // empty value is allowed
    if (value_start != value_end) {
        while (::isspace(*(value_end - 1))) {
            value_end--;
        }
    }

    _headers[std::string(key_start, key_end)] =
        std::string(value_start, value_end);

    return true;
}

bool HttpRequest::_set_method_type(const char *start, const char *end) {
    std::string method(start, end);

    if (method == "GET") {
        _method = GET;
    }

    else if (method == "POST") {
        _method = POST;
    }

    else {
        _method = UNSUPPORTED_HTTP_METHOD;
    }

    return _method != UNSUPPORTED_HTTP_METHOD;
}

bool HttpRequest::_set_version_type(const char *start, const char *end) {
    std::string version(start, end);

    if (version == "HTTP/1.0") {
        _version = HTTP_1_0;
    }

    else if (version == "HTTP/1.1") {
        _version = HTTP_1_1;
    }

    else {
        _version = UNSUPPORTED_HTTP_VERSION;
    }

    return _version != UNSUPPORTED_HTTP_VERSION;
}

const std::string HttpRequest::_empty_string{};

} // namespace xubinh_server