#ifndef __XUBINH_SERVER_HTTP_PARSER
#define __XUBINH_SERVER_HTTP_PARSER

#include <memory>
#include <stdexcept>

#include "../include/http_request.h"
#include "tcp_buffer.h"
#include "util/time_point.h"

namespace xubinh_server {

class HttpParser {
public:
    enum ParsingState {
        EXPECT_REQUEST_LINE,
        EXPECT_HEADERS,
        EXPECT_BODY,
        SUCCESS,
        FAIL,
    };

    // true = success, false = fail
    bool parse(MutableSizeTcpBuffer &buffer, const util::TimePoint &time_stamp);

    void reset() {
        _parsing_state = EXPECT_REQUEST_LINE;

        _body_length = 0;

        _request.reset();
    }

    ParsingState get_state() const {
        return _parsing_state;
    }

    bool is_success() const {
        return get_state() == SUCCESS;
    }

    const HttpRequest &get_request() const {
        if (!is_success()) {
            throw std::logic_error("HTTP request is not ready");
        }

        return _request;
    }

private:
    ParsingState _parsing_state = EXPECT_REQUEST_LINE;

    size_t _body_length = 0;

    HttpRequest _request{};
};

} // namespace xubinh_server

#endif