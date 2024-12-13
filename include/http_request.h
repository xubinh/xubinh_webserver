#ifndef XUBINH_SERVER_HTTP_REQUEST
#define XUBINH_SERVER_HTTP_REQUEST

#include <cstring>
#include <string>
#include <unordered_map>

class HttpRequest {
public:
    enum HttpMethodType { UNSUPPORTED_HTTP_METHOD, GET, POST };

    enum HttpVersionType { UNSUPPORTED_HTTP_VERSION, HTTP_1_0, HTTP_1_1 };

    // true = success, false = fail
    bool set_method_type(const char *start, const char *end) {
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

    HttpMethodType get_method_type() const {
        return _method;
    }

    const char *get_method_type_as_string() const {
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

    void set_path(const char *start, const char *end) {
        _path.assign(start, end);
    }

    const std::string &get_path() const {
        return _path;
    }

    // true = success, false = fail
    bool set_version_type(const char *start, const char *end) {
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

    HttpVersionType get_version_type() const {
        return _version;
    }

    const char *get_version_type_as_string() const {
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

    // true = success, false = fail
    bool set_header(const char *start, const char *end) {
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
    std::unordered_map<std::string, std::string> _headers;
    std::string _body;
};

#endif