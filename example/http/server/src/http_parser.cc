#include <cstring>
#include <string>

#include "../include/http_parser.h"
#include "log_builder.h"

namespace xubinh_server {

bool HttpParser::parse(
    MutableSizeTcpBuffer &buffer, const util::TimePoint &time_stamp
) {
    if (_parsing_state == FAIL) {
        return false;
    }

    if (_parsing_state == EXPECT_REQUEST_LINE) {
        auto request_line_end = buffer.get_next_crlf_position();

        if (request_line_end == nullptr) {
            return true;
        }

        auto request_line_start = buffer.get_current_read_position();

        if (!_request_ptr->parse_request_line(
                request_line_start, request_line_end
            )) {

            _parsing_state = FAIL;

            return false;
        }

        buffer.forward_read_position(
            (request_line_end + 2) - request_line_start
        );

        _parsing_state = EXPECT_HEADERS;
    }

    if (_parsing_state == EXPECT_HEADERS) {
        // loop until (1) meets incomplete line, (2) all headers are parsed, or
        // (3) fails
        while (true) {
            auto header_line_end = buffer.get_next_crlf_position();

            // (1) meets incomplete line
            if (header_line_end == nullptr) {
                return true;
            }

            auto header_line_start = buffer.get_current_read_position();

            // empty line; (2) all headers are parsed
            if (header_line_start == header_line_end) {
                buffer.forward_read_position(2);

                const auto &headers = _request_ptr->get_headers();

                auto it = headers.find("content-length");

                // check if has body (i.e. POST + "content-length" field)
                bool has_body =
                    _request_ptr->get_method_type() == _request_ptr->POST
                    && it != headers.end();

                if (!has_body) {
                    _request_ptr->set_receive_time_point(time_stamp);

                    _parsing_state = SUCCESS;

                    return true;
                }

                else {
                    _parsing_state = EXPECT_BODY;

                    _body_length = static_cast<size_t>(std::stoi(it->second));

                    goto goto_header_finished;
                }
            }

            // (3) fails
            if (!_request_ptr->parse_header_line(
                    header_line_start, header_line_end
                )) {

                _parsing_state = FAIL;

                return false;
            }

            buffer.forward_read_position(
                (header_line_end + 2) - header_line_start
            );

            continue;
        }
    }

goto_header_finished:
    if (_parsing_state == EXPECT_BODY) {
        // still haven't recieved the whole body
        if (buffer.get_readable_size() < _body_length) {
            return true;
        }

        auto body_start = buffer.get_current_read_position();

        auto body_end = body_start + _body_length;

        _request_ptr->set_body(body_start, body_end);

        buffer.forward_read_position(_body_length);

        _request_ptr->set_receive_time_point(time_stamp);

        _parsing_state = SUCCESS;

        return true;
    }

    LOG_FATAL << "unknown http parsing state";

    _parsing_state = FAIL;

    return false;
}

} // namespace xubinh_server