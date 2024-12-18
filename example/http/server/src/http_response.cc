#include "http_response.h"

namespace xubinh_server {

const char *HttpResponse::get_version_type_as_string() const {
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

const char *HttpResponse::get_status_code_and_description_as_string() const {
    const char *result = nullptr;

    switch (_status_code) {
    case 100:
        result = "100 Continue";
        break;
    case 101:
        result = "101 Switching Protocols";
        break;
    case 102:
        result = "102 Processing";
        break;
    case 200:
        result = "200 OK";
        break;
    case 201:
        result = "201 Created";
        break;
    case 202:
        result = "202 Accepted";
        break;
    case 203:
        result = "203 Non Authoritative Information";
        break;
    case 204:
        result = "204 No Content";
        break;
    case 205:
        result = "205 Reset Content";
        break;
    case 206:
        result = "206 Partial Content";
        break;
    case 207:
        result = "207 Multi Status";
        break;
    case 208:
        result = "208 Already Reported";
        break;
    case 226:
        result = "226 Im Used";
        break;
    case 300:
        result = "300 Multiple Choices";
        break;
    case 301:
        result = "301 Moved Permanently";
        break;
    case 302:
        result = "302 Found";
        break;
    case 303:
        result = "303 See Other";
        break;
    case 304:
        result = "304 Not Modified";
        break;
    case 305:
        result = "305 Use Proxy";
        break;
    case 307:
        result = "307 Temporary Redirect";
        break;
    case 308:
        result = "308 Permanent Redirect";
        break;
    case 400:
        result = "400 Bad Request";
        break;
    case 401:
        result = "401 Unauthorized";
        break;
    case 402:
        result = "402 Payment Required";
        break;
    case 403:
        result = "403 Forbidden";
        break;
    case 404:
        result = "404 Not Found";
        break;
    case 405:
        result = "405 Method Not Allowed";
        break;
    case 406:
        result = "406 Not Acceptable";
        break;
    case 407:
        result = "407 Proxy Authentication Required";
        break;
    case 408:
        result = "408 Request Timeout";
        break;
    case 409:
        result = "409 Conflict";
        break;
    case 410:
        result = "410 Gone";
        break;
    case 411:
        result = "411 Length Required";
        break;
    case 412:
        result = "412 Precondition Failed";
        break;
    case 413:
        result = "413 Payload Too Large";
        break;
    case 414:
        result = "414 URI Too Long";
        break;
    case 415:
        result = "415 Unsupported Media Type";
        break;
    case 416:
        result = "416 Range Not Satisfiable";
        break;
    case 417:
        result = "417 Expectation Failed";
        break;
    case 418:
        result = "418 Im A Teapot";
        break;
    case 421:
        result = "421 Misdirected Request";
        break;
    case 422:
        result = "422 Unprocessable Entity";
        break;
    case 423:
        result = "423 Locked";
        break;
    case 424:
        result = "424 Failed Dependency";
        break;
    case 425:
        result = "425 Too Early";
        break;
    case 426:
        result = "426 Upgrade Required";
        break;
    case 428:
        result = "428 Precondition Required";
        break;
    case 429:
        result = "429 Too Many Requests";
        break;
    case 431:
        result = "431 Request Header Fields Too Large";
        break;
    case 451:
        result = "451 Unavailable For Legal Reasons";
        break;
    case 500:
        result = "500 Internal Server Error";
        break;
    case 501:
        result = "501 Not Implemented";
        break;
    case 502:
        result = "502 Bad Gateway";
        break;
    case 503:
        result = "503 Service Unavailable";
        break;
    case 504:
        result = "504 Gateway Timeout";
        break;
    case 505:
        result = "505 HTTP Version Not Supported";
        break;
    case 506:
        result = "506 Variant Also Negotiates";
        break;
    case 507:
        result = "507 Insufficient Storage";
        break;
    case 508:
        result = "508 Loop Detected";
        break;
    case 510:
        result = "510 Not Extended";
        break;
    case 511:
        result = "511 Network Authentication Required";
        break;
    default:
        result = "unknown http status code";
        break;
    }

    return result;
}

const std::string &HttpResponse::get_header(const std::string &key) const {
    auto it = _headers.find(key);

    if (it != _headers.end()) {
        return it->second;
    }

    return _empty_string;
}

bool HttpResponse::erase_header(const std::string &key) {
    auto it = _headers.find(key);

    if (it != _headers.end()) {
        _headers.erase(it);

        return true;
    }

    return false;
}

void HttpResponse::dump_to_tcp_buffer(MutableSizeTcpBuffer &buffer) {
    if (_status_code == S_NONE) {
        LOG_FATAL << "tried to dump a http response before setting the "
                     "status code";
    }

    auto version_str = get_version_type_as_string();

    buffer.write(version_str, ::strlen(version_str));

    buffer.write_space();

    auto status_code_and_description_str =
        get_status_code_and_description_as_string();

    buffer.write(
        status_code_and_description_str,
        ::strlen(status_code_and_description_str)
    );

    buffer.write_crlf();

    for (const auto &key_value_pair : _headers) {
        buffer.write(
            key_value_pair.first.c_str(), key_value_pair.first.length()
        );

        buffer.write_colon();

        buffer.write(
            key_value_pair.second.c_str(), key_value_pair.second.length()
        );

        buffer.write_crlf();
    }

    // an empty line is required to end the header section
    buffer.write_crlf();

    if (!_body.empty()) {
        buffer.write(_body.c_str(), _body.length());
    }
}

} // namespace xubinh_server