#include <csignal>
#include <memory>
#include <string>
#include <unistd.h>

#include "event_loop.h"
#include "inet_address.h"
#include "log_collector.h"
#include "signalfd.h"
#include "tcp_client.h"
#include "util/datetime.h"

#include "./include/stdinfd.h"

using Stdinfd = xubinh_server::Stdinfd;

using TcpConnectSocketfdPtr = xubinh_server::TcpClient::TcpConnectSocketfdPtr;

std::unique_ptr<xubinh_server::Signalfd>
    signalfd_ptr;                   // for lazy initialization
std::unique_ptr<Stdinfd> stdin_ptr; // for lazy initialization

void signal_dispatcher(
    xubinh_server::TcpClient *client, xubinh_server::EventLoop *loop, int signal
) {
    switch (signal) {
    case SIGHUP:
    case SIGINT:
    case SIGQUIT:
    case SIGTERM:
    case SIGTSTP:
        LOG_INFO << "user interrupt, terminating client...";

        stdin_ptr.reset();

        signalfd_ptr->stop();

        client->stop();

        // loop will only stop when all fd's are detached from it
        loop->ask_to_stop();

        break;

    default:
        LOG_FATAL << "unknown signal encountered";

        break;
    }
}

void message_callback(
    const TcpConnectSocketfdPtr &tcp_connect_socketfd_ptr,
    xubinh_server::MutableSizeTcpBuffer *input_buffer,
    xubinh_server::util::TimePoint time_point
) {
    while (true) {
        auto newline_position = input_buffer->get_next_newline_position();

        if (!newline_position) {
            break;
        }

        const char *server_message_end = newline_position + 1;

        const char *server_message_start =
            const_cast<const char *>(input_buffer->get_current_read_position());

        std::string server_message =
            std::string(server_message_start, server_message_end);

        std::cout << server_message << std::flush;

        input_buffer->forward_read_position(
            server_message_end - server_message_start
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

    // create client
    xubinh_server::InetAddress server_address(
        "127.0.0.1", 8080, xubinh_server::InetAddress::IPv4
    );
    xubinh_server::TcpClient client(&loop, server_address);

    // signalfd config
    signalfd_ptr.reset(new xubinh_server::Signalfd(
        xubinh_server::Signalfd::create_signalfd(signal_set, 0), &loop
    ));
    signalfd_ptr->register_signal_dispatcher([&](int signal) {
        signal_dispatcher(&client, &loop, signal);
    });
    signalfd_ptr->start();

    // client config
    client.register_message_callback(message_callback);
    client.register_connect_success_callback([&](const TcpConnectSocketfdPtr &
                                                     tcp_connect_socketfd_ptr) {
        LOG_TRACE
            << "established TCP connection (id: "
            << tcp_connect_socketfd_ptr->get_id()
            << "): " + tcp_connect_socketfd_ptr->get_local_address().to_string()
                   + " -> "
                   + tcp_connect_socketfd_ptr->get_remote_address().to_string();

        stdin_ptr.reset(new Stdinfd(&loop, tcp_connect_socketfd_ptr));

        stdin_ptr->start();
    });
    client.register_connect_fail_callback([]() {
        LOG_ERROR << "connection failed";
    });

    client.start();

    loop.loop();

    return 0;
}