#include <csignal>
#include <memory>
#include <string>
#include <unistd.h>

#include "event_loop.h"
#include "inet_address.h"
#include "log_builder.h"
#include "log_collector.h"
#include "signalfd.h"
#include "tcp_client.h"
#include "util/datetime.h"

#include "./include/stdinfd.h"

using Stdinfd = xubinh_server::Stdinfd;

using TcpConnectSocketfdPtr = xubinh_server::TcpClient::TcpConnectSocketfdPtr;

// manually release the singleton logger instance
xubinh_server::LogCollector::CleanUpHelper logger_clean_up_hook;

std::unique_ptr<xubinh_server::Signalfd>
    signalfd_ptr; // for lazy initialization

std::unique_ptr<Stdinfd> stdin_ptr; // for lazy initialization

void signal_dispatcher(
    xubinh_server::TcpClient *client, xubinh_server::EventLoop *loop, int signal
) {
    switch (signal) {
    case SIGTSTP:
        // go to background like normal; don't do anything
        return;

    case SIGHUP:
    case SIGINT:
    case SIGQUIT:
    case SIGTERM:
        LOG_INFO << "user interrupt, terminating client...";

        stdin_ptr.reset();

        // [NOTE]: must unregister signalfd first for the loop to be able to
        // stop, since signalfd is not counted into the resident fd's of the
        // event loop
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
    __attribute__((unused))
    xubinh_server::TcpConnectSocketfd *tcp_connect_socketfd_ptr,
    xubinh_server::MutableSizeTcpBuffer *input_buffer,
    __attribute__((unused)) xubinh_server::util::TimePoint time_point
) {
    while (true) {
        auto newline_position = input_buffer->get_next_newline_position();

        if (!newline_position) {
            break;
        }

        const char *server_message_end = newline_position + 1;
        const char *server_message_start = input_buffer->get_read_position();
        size_t server_message_length =
            server_message_end - server_message_start;

        std::string server_message =
            std::string(server_message_start, server_message_length);

        std::cout << server_message << std::flush;

        input_buffer->forward_read_position(server_message_length);
    }
}

int main() {
    // signal config
    xubinh_server::SignalSet signal_set;
    signal_set.add_signal(SIGHUP);
    signal_set.add_signal(SIGINT);
    signal_set.add_signal(SIGQUIT);
    signal_set.add_signal(SIGTERM);
    signal_set.add_signal(SIGTSTP);
    xubinh_server::Signalfd::block_signals(signal_set);
    // [NOTE]: always block signals first so that worker threads can be spawn
    // without worries

    // logging config
    xubinh_server::LogCollector::set_base_name("echo-client");
    xubinh_server::LogCollector::set_if_need_output_directly_to_terminal(false);
    xubinh_server::LogBuilder::set_log_level(xubinh_server::LogLevel::FATAL);
    // [NOTE]: logging settings should also be configured as soon as possible so
    // that others can emit logs out without worries

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
            << "TCP connection established, id: "
            << tcp_connect_socketfd_ptr->get_id()
            << ", path: "
                   + tcp_connect_socketfd_ptr->get_local_address().to_string()
                   + " -> "
                   + tcp_connect_socketfd_ptr->get_remote_address().to_string();

        stdin_ptr.reset(new Stdinfd(&loop, tcp_connect_socketfd_ptr));

        stdin_ptr->start();
    });
    client.register_connect_fail_callback([]() {
        LOG_ERROR << "connection failed";
    });
    client.start();
    LOG_INFO << "client started connecting to " + server_address.to_string();
    xubinh_server::LogCollector::flush();

    loop.loop();

    LOG_INFO << "test finished";

    return 0;
}