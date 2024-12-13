#include "http_request.h"

namespace xubinh_server {

bool HttpRequest::set_method_type(const char *start, const char *end) {
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

const char *HttpRequest::get_method_type_as_string() const {
    switch (_method) {

    case GET: {
        return "GET";
        break;
    }

    case POST: {
        return "POST";
        break;
    }

    default: {
        return nullptr;
        break;
    }
    }
}

bool HttpRequest::set_version_type(const char *start, const char *end) {
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

const char *HttpRequest::get_version_type_as_string() const {
    switch (_version) {

    case HTTP_1_0: {
        return "HTTP/1.0";
        break;
    }

    case HTTP_1_1: {
        return "HTTP/1.1";
        break;
    }

    default: {
        return nullptr;
        break;
    }
    }
}

bool HttpRequest::set_header(const char *start, const char *end) {
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

    const char *value_start = colon + 1;
    const char *value_end = end;

    while (value_start < value_end && ::isspace(*value_start)) {
        value_start++;
    }

    if (value_start != value_end) {
        while (::isspace(*(value_end - 1))) {
            value_end--;
        }
    }

    _headers[std::string(key_start, key_end)] =
        std::string(value_start, value_end);

    return true;
}

} // namespace xubinh_server