#include <csignal>
#include <string>

#include "event_loop.h"
#include "inet_address.h"
#include "log_builder.h"
#include "log_collector.h"
#include "signalfd.h"
#include "tcp_server.h"
#include "util/datetime.h"

using TcpConnectSocketfdPtr = xubinh_server::TcpServer::TcpConnectSocketfdPtr;

std::unique_ptr<xubinh_server::Signalfd>
    signalfd_ptr; // for lazy initialization

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
        LOG_INFO << "user interrupt, terminating server...";

        signalfd_ptr->stop();

        server->stop();

        // loop will only stop when all fd's are detached from it
        loop->ask_to_stop();

        break;

    default:
        LOG_FATAL << "unknown signal encountered";

        break;
    }
}

void message_callback(
    xubinh_server::TcpConnectSocketfd *tcp_connect_socketfd_ptr,
    xubinh_server::MutableSizeTcpBuffer *input_buffer,
    xubinh_server::util::TimePoint time_point
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
            xubinh_server::util::TimePoint::Purpose::PRINTING
        );

        // auto msg = "[" + time_stamp + "]: " + client_message;
        auto msg = client_message;

        tcp_connect_socketfd_ptr->send(msg.c_str(), msg.length());

        LOG_TRACE << "send message: "
                         + std::string(
                             msg.c_str(), msg.c_str() + msg.length() - 1
                         );

        input_buffer->forward_read_position(
            client_message_end - client_message_start
        );
    }
}

int main() {
    // logging config
    xubinh_server::LogCollector::set_if_need_output_directly_to_terminal(true);
    xubinh_server::LogBuilder::set_log_level(xubinh_server::LogLevel::TRACE);

    // signal config
    xubinh_server::SignalSet signal_set;
    signal_set.add_signal(SIGHUP);
    signal_set.add_signal(SIGINT);
    signal_set.add_signal(SIGQUIT);
    signal_set.add_signal(SIGTERM);
    signal_set.add_signal(SIGTSTP);
    xubinh_server::Signalfd::block_signals(signal_set);

    // create loop
    xubinh_server::EventLoop loop;

    // create server
    xubinh_server::InetAddress server_address(
        "127.0.0.1", 8080, xubinh_server::InetAddress::IPv4
    );
    xubinh_server::TcpServer server(&loop, server_address);

    // signalfd config
    signalfd_ptr.reset(new xubinh_server::Signalfd(
        xubinh_server::Signalfd::create_signalfd(signal_set, 0), &loop
    ));
    signalfd_ptr->register_signal_dispatcher([&](int signal) {
        signal_dispatcher(&server, &loop, signal);
    });
    signalfd_ptr->start();

    // server config
    server.register_message_callback(message_callback);
    server.register_connect_success_callback([](const TcpConnectSocketfdPtr
                                                    &tcp_connect_socketfd_ptr) {
        LOG_TRACE
            << "established TCP connection (id: "
            << tcp_connect_socketfd_ptr->get_id()
            << "): " + tcp_connect_socketfd_ptr->get_local_address().to_string()
                   + " -> "
                   + tcp_connect_socketfd_ptr->get_remote_address().to_string();
    });
    server.set_thread_pool_capacity(1);

    server.start();

    LOG_INFO << "server started listening on " + server_address.to_string();

    loop.loop();

    return 0;
}