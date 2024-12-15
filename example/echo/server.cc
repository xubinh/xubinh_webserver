#include <csignal>
#include <string>

#include "event_loop.h"
#include "inet_address.h"
#include "signalfd.h"
#include "tcp_server.h"
#include "util/datetime.h"

// placeholder
void reload_server_config(xubinh_server::TcpServer *server) {
}

void signal_dispatcher(
    xubinh_server::TcpServer *server, xubinh_server::EventLoop *loop, int signal
) {
    switch (signal) {

    case SIGHUP:
        reload_server_config(server);

        break;

    case SIGINT:
    case SIGQUIT:
    case SIGTERM:
    case SIGTSTP:
        loop->ask_to_stop();

        break;

    default:
        LOG_FATAL << "unknown signal encountered";

        break;
    }
}

using TcpConnectSocketfdPtr = xubinh_server::TcpServer::TcpConnectSocketfdPtr;

void message_callback(
    const TcpConnectSocketfdPtr &tcp_connect_socketfd_ptr,
    xubinh_server::MutableSizeTcpBuffer *input_buffer,
    const xubinh_server::util::TimePoint &time_point
) {
    while (true) {
        auto newline_position = input_buffer->get_next_newline_position();

        if (!newline_position) {
            break;
        }

        const char *client_message_end = newline_position + 1;

        const char *client_message_start =
            const_cast<const char *>(input_buffer->get_current_read_position());

        std::string client_message =
            std::string(client_message_start, client_message_end);

        std::string time_stamp = time_point.to_datetime_string(
            xubinh_server::util::TimePoint::DatetimePurpose::PRINTING
        );

        auto msg = "[" + time_stamp + "]: " + client_message;

        tcp_connect_socketfd_ptr->send(msg.c_str(), msg.length());

        input_buffer->forward_read_position(
            client_message_end - client_message_start
        );
    }
}

int main() {
    xubinh_server::SignalSet signal_set;

    signal_set.add_signal(SIGHUP);
    signal_set.add_signal(SIGINT);
    signal_set.add_signal(SIGQUIT);
    signal_set.add_signal(SIGTERM);
    signal_set.add_signal(SIGTSTP);

    xubinh_server::Signalfd::block_signals(signal_set);

    xubinh_server::EventLoop loop;

    xubinh_server::InetAddress server_address(
        "127.0.0.1", 8080, xubinh_server::InetAddress::IPv4
    );

    xubinh_server::TcpServer server(&loop, server_address);

    xubinh_server::Signalfd signalfd(
        xubinh_server::Signalfd::create_signalfd(signal_set, 0), &loop
    );

    signalfd.register_signal_dispatcher(
        std::bind(signal_dispatcher, &server, &loop, std::placeholders::_1)
    );

    signalfd.start();

    server.register_message_callback(message_callback);

    server.set_thread_pool_capacity(4);

    server.start();

    loop.loop();

    server.stop();

    return 0;
}