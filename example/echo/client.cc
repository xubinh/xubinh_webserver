#include <csignal>
#include <string>
#include <unistd.h>

#include "event_loop.h"
#include "inet_address.h"
#include "signalfd.h"
#include "tcp_client.h"
#include "util/datetime.h"

void signal_dispatcher(xubinh_server::EventLoop *loop, int signal) {
    switch (signal) {
    case SIGHUP:
    case SIGINT:
    case SIGQUIT:
    case SIGTERM:
    case SIGTSTP:
        loop->ask_to_stop();

        std::cout << "---------------- Session End ----------------"
                  << std::endl;

        break;

    default:
        LOG_FATAL << "unknown signal encountered";

        break;
    }
}

using TcpConnectSocketfdPtr = xubinh_server::TcpClient::TcpConnectSocketfdPtr;

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

using TcpConnectSocketfdPtr = xubinh_server::TcpClient::TcpConnectSocketfdPtr;

class Stdin {
public:
    Stdin(
        xubinh_server::EventLoop *loop,
        const TcpConnectSocketfdPtr &tcp_connect_socketfd_ptr
    )
        : _tcp_connect_socketfd_weak_ptr(tcp_connect_socketfd_ptr),
          _pollable_file_descriptor(STDIN_FILENO, loop) {

        _pollable_file_descriptor.register_read_event_callback(
            std::bind(&Stdin::_read_event_callback, this)
        );

        _pollable_file_descriptor.enable_read_event();

        std::cout << "---------------- Session Begin ----------------"
                  << std::endl;
    }

    ~Stdin() {
        _pollable_file_descriptor.disable_read_event();
    }

private:
    void _read_event_callback() {
        auto _tcp_connect_socketfd_ptr = _tcp_connect_socketfd_weak_ptr.lock();

        if (!_tcp_connect_socketfd_ptr) {
            LOG_ERROR << "connection closed";

            _pollable_file_descriptor.disable_read_event();

            return;
        }

        while (true) {
            char buffer[256];

            ssize_t bytes_read = ::read(STDIN_FILENO, buffer, sizeof(buffer));

            if (bytes_read > 0) {
                _tcp_connect_socketfd_ptr->send(buffer, bytes_read);

                continue;
            }

            else if (bytes_read == 0) {
                LOG_FATAL << "stdin closed";
            }

            else {
                if (errno == EAGAIN) {
                    break;
                }

                else {
                    LOG_SYS_FATAL << "unknown error when reading from stdin";
                }
            }
        }
    }

    std::weak_ptr<TcpConnectSocketfdPtr::element_type>
        _tcp_connect_socketfd_weak_ptr;

    xubinh_server::PollableFileDescriptor _pollable_file_descriptor;
};

void connect_success_callback(
    std::shared_ptr<Stdin> stdin_ptr,
    xubinh_server::EventLoop *loop,
    const TcpConnectSocketfdPtr &tcp_connect_socketfd_ptr
) {
    stdin_ptr.reset(new Stdin(loop, tcp_connect_socketfd_ptr));
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

    xubinh_server::TcpClient client(&loop, server_address);

    xubinh_server::Signalfd signalfd(
        xubinh_server::Signalfd::create_signalfd(signal_set, 0), &loop
    );

    signalfd.register_signal_dispatcher(
        std::bind(signal_dispatcher, &loop, std::placeholders::_1)
    );

    signalfd.start();

    std::shared_ptr<Stdin> stdin_ptr;

    client.register_connect_success_callback(std::bind(
        connect_success_callback, stdin_ptr, &loop, std::placeholders::_1
    ));

    client.register_message_callback(message_callback);

    client.register_connect_fail_callback([]() {
        LOG_ERROR << "connection timeout";
    });

    client.start();

    loop.loop();

    return 0;
}