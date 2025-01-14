#ifndef __XUBINH_SERVER_HTTP_RESPONSE
#define __XUBINH_SERVER_HTTP_RESPONSE

#include <string>
#include <unordered_map>
#include <vector>

#include "tcp_buffer.h"
#include "tcp_connect_socketfd.h"
#include "util/slab_allocator.h"

namespace xubinh_server {

class HttpResponse {
public:
    using StringType = util::StringType;

    enum HttpVersionType { UNSUPPORTED_HTTP_VERSION, HTTP_1_0, HTTP_1_1 };

    enum HttpStatusCode {
        S_NONE,

        S_100_CONTINUE = 100,
        S_101_SWITCHING_PROTOCOLS = 101,
        S_102_PROCESSING = 102,

        S_200_OK = 200,
        S_201_CREATED = 201,
        S_202_ACCEPTED = 202,
        S_203_NON_AUTHORITATIVE_INFORMATION = 203,
        S_204_NO_CONTENT = 204,
        S_205_RESET_CONTENT = 205,
        S_206_PARTIAL_CONTENT = 206,
        S_207_MULTI_STATUS = 207,
        S_208_ALREADY_REPORTED = 208,
        S_226_IM_USED = 226,

        S_300_MULTIPLE_CHOICES = 300,
        S_301_MOVED_PERMANENTLY = 301,
        S_302_FOUND = 302,
        S_303_SEE_OTHER = 303,
        S_304_NOT_MODIFIED = 304,
        S_305_USE_PROXY = 305,
        S_307_TEMPORARY_REDIRECT = 307,
        S_308_PERMANENT_REDIRECT = 308,

        S_400_BAD_REQUEST = 400,
        S_401_UNAUTHORIZED = 401,
        S_402_PAYMENT_REQUIRED = 402,
        S_403_FORBIDDEN = 403,
        S_404_NOT_FOUND = 404,
        S_405_METHOD_NOT_ALLOWED = 405,
        S_406_NOT_ACCEPTABLE = 406,
        S_407_PROXY_AUTHENTICATION_REQUIRED = 407,
        S_408_REQUEST_TIMEOUT = 408,
        S_409_CONFLICT = 409,
        S_410_GONE = 410,
        S_411_LENGTH_REQUIRED = 411,
        S_412_PRECONDITION_FAILED = 412,
        S_413_PAYLOAD_TOO_LARGE = 413,
        S_414_URI_TOO_LONG = 414,
        S_415_UNSUPPORTED_MEDIA_TYPE = 415,
        S_416_RANGE_NOT_SATISFIABLE = 416,
        S_417_EXPECTATION_FAILED = 417,
        S_418_IM_A_TEAPOT = 418,
        S_421_MISDIRECTED_REQUEST = 421,
        S_422_UNPROCESSABLE_ENTITY = 422,
        S_423_LOCKED = 423,
        S_424_FAILED_DEPENDENCY = 424,
        S_425_TOO_EARLY = 425,
        S_426_UPGRADE_REQUIRED = 426,
        S_428_PRECONDITION_REQUIRED = 428,
        S_429_TOO_MANY_REQUESTS = 429,
        S_431_REQUEST_HEADER_FIELDS_TOO_LARGE = 431,
        S_451_UNAVAILABLE_FOR_LEGAL_REASONS = 451,

        S_500_INTERNAL_SERVER_ERROR = 500,
        S_501_NOT_IMPLEMENTED = 501,
        S_502_BAD_GATEWAY = 502,
        S_503_SERVICE_UNAVAILABLE = 503,
        S_504_GATEWAY_TIMEOUT = 504,
        S_505_HTTP_VERSION_NOT_SUPPORTED = 505,
        S_506_VARIANT_ALSO_NEGOTIATES = 506,
        S_507_INSUFFICIENT_STORAGE = 507,
        S_508_LOOP_DETECTED = 508,
        S_510_NOT_EXTENDED = 510,
        S_511_NETWORK_AUTHENTICATION_REQUIRED = 511,
    };

    HttpResponse() = default;

    // no copy
    HttpResponse(const HttpResponse &) = delete;
    HttpResponse &operator=(const HttpResponse &) = delete;

    // no move
    HttpResponse(HttpResponse &&) = delete;
    HttpResponse &operator=(HttpResponse &&) = delete;

    ~HttpResponse() = default;

    void set_version_type(HttpVersionType version_type) {
        _version = version_type;
    }

    HttpVersionType get_version_type() const {
        return _version;
    }

    const char *get_version_type_as_string() const;

    void set_status_code(HttpStatusCode status_code) {
        _status_code = status_code;
    }

    HttpStatusCode get_status_code() const {
        return _status_code;
    }

    const char *get_status_code_and_description_as_string() const;

    void set_header(const StringType &key, const StringType &value) {
        _headers[key] = value;
    }

    // [WARN]: always use this single-threadedly
    void set_header(const StringType &key, StringType &&value) {
        _headers[key] = std::move(value);
    }

    // adaptor for accepting `char[N]`
    template <size_t N>
    void set_header(const StringType &key, const char value[N]) {
        _headers[key] = StringType(value, N);
    }

    // adaptor for accepting `std::string`
    template <
        typename AnotherStringType,
        typename = typename std::enable_if<
            std::is_same<std::string, AnotherStringType>::value
            && !std::is_same<StringType, AnotherStringType>::value>::type>
    void set_header(const StringType &key, const AnotherStringType &value) {
        _headers[key] = StringType(value.c_str(), value.size());
    }

    const StringType &get_header(const StringType &key) const;

    // true = found, false = not found
    bool erase_header(const StringType &key);

    void set_body(const StringType &body) {
        _body = body;

        set_header("Content-Length", util::to_string<StringType>(body.size()));
    }

    void set_body(StringType &&body) {
        _body = std::move(body);

        set_header("Content-Length", util::to_string<StringType>(body.size()));
    }

    void set_body(const char *start, const char *end) {
        _body.assign(start, end);

        set_header("Content-Length", util::to_string<StringType>(end - start));
    }

    void dump_to_tcp_buffer(MutableSizeTcpBuffer &buffer);

    void send_to_tcp_connection(TcpConnectSocketfd *tcp_connect_socketfd_ptr);

private:
    static const StringType _empty_string;

    HttpVersionType _version;
    HttpStatusCode _status_code = S_NONE;
    std::unordered_map<StringType, StringType> _headers;
    StringType _body;
};

} // namespace xubinh_server

#endif