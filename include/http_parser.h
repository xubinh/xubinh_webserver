#ifndef XUBINH_SERVER_HTTP_PARSER
#define XUBINH_SERVER_HTTP_PARSER

#include <memory>

#include "http_request.h"
#include "tcp_buffer.h"
#include "util/time_point.h"

namespace xubinh_server {

class HttpParser {
public:
    enum ParsingState {
        EXPECT_REQUEST_LINE,
        EXPECT_HEADERS,
        EXPECT_BODY,
        SUCCESS
    };

    // true = success, false = fail
    bool parse(
        MutableSizeTcpBuffer &buffer, const util::TimePoint &receive_time_point
    );

    void reset() {
        _parsing_state = EXPECT_REQUEST_LINE;

        _request_ptr.reset(new HttpRequest{});
    }

private:
    ParsingState _parsing_state = EXPECT_REQUEST_LINE;

    size_t _body_length = 0;

    std::unique_ptr<HttpRequest> _request_ptr{new HttpRequest{}};
};

} // namespace xubinh_server

#endif