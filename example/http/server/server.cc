#include <csignal>
#include <cstring>
#include <fcntl.h>
#include <string>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <unordered_map>

#include "event_loop.h"
#include "inet_address.h"
#include "log_builder.h"
#include "log_collector.h"
#include "signalfd.h"
#include "tcp_buffer.h"
#include "util/datetime.h"
#include "util/slab_allocator.h"

#include "./include/http_parser.h"
#include "./include/http_request.h"
#include "./include/http_response.h"
#include "./include/http_server.h"

using TcpConnectSocketfdPtr = xubinh_server::HttpServer::TcpConnectSocketfdPtr;

using StringType = xubinh_server::util::StringType;

// manually release the singleton logger instance
xubinh_server::LogCollector::CleanUpHelper logger_clean_up_hook;

std::unique_ptr<xubinh_server::Signalfd>
    signalfd_ptr; // for lazy initialization

// placeholder
void reload_server_config(__attribute__((unused))
                          xubinh_server::HttpServer *server) {
}

void signal_dispatcher(
    xubinh_server::HttpServer *server,
    xubinh_server::EventLoop *loop,
    int signal
) {
    switch (signal) {
    case SIGTSTP:
        // go to background like normal; don't do anything
        return;

    case SIGHUP:
        reload_server_config(server);

        break;

    case SIGINT:
    case SIGQUIT:
    case SIGTERM:
        LOG_INFO << "user interrupt, terminating server...";

        // [NOTE]: must unregister signalfd first for the loop to be able to
        // stop, since signalfd is not counted into the resident fd's of the
        // event loop
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

const char *slash_dot_dot_slash = "/../";
constexpr size_t slash_dot_dot_slash_len = sizeof(slash_dot_dot_slash);

bool check_if_is_valid_path(const StringType &path) {
    const char *path_str = path.c_str();

    size_t path_str_len = path.length();

    if (path_str[0] != '/') {
        return false;
    }

    if (std::search(
            path_str,
            path_str + path_str_len,
            slash_dot_dot_slash,
            slash_dot_dot_slash + slash_dot_dot_slash_len
        )
        != path_str + path_str_len) {
        return false;
    }

    return true;
}

// [TODO]: use stand-alone config file, not hard-coded one
const char *root = "./example/http/server";
const char *html_404_file_path = "./example/http/server/404.html";
const char *images_folder = "/static/images/";
constexpr size_t images_folder_len = sizeof(images_folder);

bool _check_if_path_exists(const StringType &path) {
    struct stat info;

    return stat(path.c_str(), &info) == 0;
}

bool check_if_path_exists_and_expand_it(StringType &path) {
    if (path == "/") {
        path = root + path;
        path += "index.html";

        return true;
    }

    if (path == "/index.html") {
        return true;
    }

    if (::strncmp(path.c_str(), images_folder, images_folder_len) != 0) {
        return false;
    }

    path = root + path;

    return _check_if_path_exists(path);
}

std::unordered_map<std::string, std::string> mime_map = {
    {".html", "text/html"},
    {".htm", "text/html"},
    {".css", "text/css"},
    {".js", "application/javascript"},
    {".json", "application/json"},
    {".png", "image/png"},
    {".jpg", "image/jpeg"},
    {".jpeg", "image/jpeg"},
    {".gif", "image/gif"},
    {".txt", "text/plain"},
    {".xml", "application/xml"},
    {".pdf", "application/pdf"},
    {".zip", "application/zip"},
    {".mp3", "audio/mpeg"},
    {".mp4", "video/mp4"}};

const std::string default_mime_type = "application/octet-stream";

const std::string &get_mime_type(const StringType &file_path) {
    size_t pos = file_path.rfind('.');

    if (pos == std::string::npos) {
        return default_mime_type;
    }

    std::string suffix(file_path.c_str() + pos, file_path.size() - pos);

    auto it = mime_map.find(suffix);

    if (it != mime_map.end()) {
        return it->second;
    }

    return default_mime_type;
}

bool read_file_and_send(
    xubinh_server::TcpConnectSocketfd *tcp_connect_socketfd_ptr,
    xubinh_server::HttpResponse &response,
    const StringType &file_path
) {
    int fd = ::open(file_path.c_str(), O_RDONLY);

    if (fd == -1) {
        LOG_SYS_ERROR << "file not found, path: " << file_path;

        return false;
    }

    struct stat st;

    if (::fstat(fd, &st) == -1) {
        LOG_SYS_ERROR << "failed to get file stats, path: " << file_path;

        ::close(fd);

        return false;
    }

    size_t file_size = st.st_size;

    response.set_header(
        "Content-Length", xubinh_server::util::to_string<StringType>(file_size)
    );

    response.send_to_tcp_connection(tcp_connect_socketfd_ptr);

    char *mapped_data = static_cast<char *>(
        ::mmap(nullptr, file_size, PROT_READ, MAP_PRIVATE, fd, 0)
    );

    if (mapped_data == MAP_FAILED) {
        LOG_SYS_ERROR << "failed to mmap file, path: " << file_path;

        ::close(fd);

        return false;
    }

    tcp_connect_socketfd_ptr->send(mapped_data, file_size);

    if (::munmap(mapped_data, file_size) == -1) {
        LOG_SYS_ERROR << "failed to munmap file, path: " << file_path;

        ::close(fd);

        return false;
    }

    ::close(fd);

    return true;
}

void send_404(
    xubinh_server::TcpConnectSocketfd *tcp_connect_socketfd_ptr, bool need_close
) {
    xubinh_server::HttpResponse response;

    response.set_version_type(xubinh_server::HttpResponse::HTTP_1_1);
    response.set_status_code(xubinh_server::HttpResponse::S_404_NOT_FOUND);
    response.set_header("Content-Type", "text/html; charset=UTF-8");

    if (need_close) {
        response.set_header("Connection", "close");
    }

    if (!read_file_and_send(
            tcp_connect_socketfd_ptr, response, html_404_file_path
        )) {
        LOG_ERROR << "failed to send file, path: " << html_404_file_path;
    }
}

void send_file(
    xubinh_server::TcpConnectSocketfd *tcp_connect_socketfd_ptr,
    bool need_close,
    const StringType &file_path
) {
    xubinh_server::HttpResponse response;

    response.set_version_type(xubinh_server::HttpResponse::HTTP_1_1);
    response.set_status_code(xubinh_server::HttpResponse::S_200_OK);
    response.set_header("Content-Type", get_mime_type(file_path));

    if (need_close) {
        response.set_header("Connection", "close");
    }

    if (!read_file_and_send(tcp_connect_socketfd_ptr, response, file_path)) {
        LOG_ERROR << "failed to send file, path: " << file_path;
    }
}

// const char hello_world_response_content[] =
//     "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nConnection: "
//     "close\r\nContent-Length: 115\r\n\r\n<!DOCTYPE html><html "
//     "lang=\"en\"><head><title>Hello, "
//     "world!</title></head><body><h1>Hello, world!</h1></body></html>";
const char hello_world_response_content[] =
    "HTTP/1.1 200 OK\r\nContent-type: text/plain\r\n\r\nHello World";
// [NOTE]: for comparing with <https://github.com/linyacool/WebServer>

constexpr size_t hello_world_response_content_size =
    sizeof(hello_world_response_content);

void http_request_callback(
    xubinh_server::TcpConnectSocketfd *tcp_connect_socketfd_ptr,
#ifdef __HTTP_EXAMPLE_RUN_BENCHMARK
    __attribute__((unused))
#endif
    const xubinh_server::HttpRequest &http_request
) {
    if (tcp_connect_socketfd_ptr->is_write_end_shutdown()) {
        return;
    }

#ifdef __HTTP_EXAMPLE_RUN_BENCHMARK
    // directly sends the string in memory, bypassing mmap
    tcp_connect_socketfd_ptr->send(
        hello_world_response_content, hello_world_response_content_size
    );
#else
    bool need_close = http_request.get_need_close();

    if (http_request.get_method_type() != xubinh_server::HttpRequest::GET) {
        send_404(tcp_connect_socketfd_ptr, need_close);

        return;
    }

    auto path = http_request.get_path();

    if (!check_if_is_valid_path(path)
        || !check_if_path_exists_and_expand_it(path)) {
        send_404(tcp_connect_socketfd_ptr, need_close);

        return;
    }

    send_file(tcp_connect_socketfd_ptr, need_close, path);
#endif
}

int main(int argc, char *argv[]) {
    // this is for benchmarking
    if (argc > 2) {
        ::fprintf(stderr, "Usage: %s [log_file_base_name]\n", argv[0]);

        ::exit(1);
    }

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
    std::string log_file_base_name =
        (argc == 2 ? std::string(argv[1]) : "http-server");
    xubinh_server::LogCollector::set_base_name(log_file_base_name);
#ifdef __HTTP_EXAMPLE_RUN_BENCHMARK
    xubinh_server::LogCollector::set_if_need_output_directly_to_terminal(false);
    xubinh_server::LogBuilder::set_log_level(xubinh_server::LogLevel::FATAL);
#else
    xubinh_server::LogCollector::set_if_need_output_directly_to_terminal(false);
    xubinh_server::LogBuilder::set_log_level(xubinh_server::LogLevel::INFO);
#endif
    // [NOTE]: logging settings should also be configured as soon as possible so
    // that others can emit logs out without worries

    // fd limit config
    xubinh_server::EventPoller::
        set_limit_of_max_number_of_opened_file_descriptors_per_process(
            static_cast<size_t>(60000)
        );

    size_t thread_pool_capacity = 4;

    // create loop
    size_t number_of_functor_blocking_queues = thread_pool_capacity;
    xubinh_server::EventLoop loop(0, number_of_functor_blocking_queues);

    // create server
    xubinh_server::InetAddress server_address(
        "127.0.0.1", 8080, xubinh_server::InetAddress::IPv4
    );
    xubinh_server::HttpServer server(&loop, server_address);

    // signalfd config
    signalfd_ptr.reset(new xubinh_server::Signalfd(
        xubinh_server::Signalfd::create_signalfd(signal_set, 0), &loop
    ));
    signalfd_ptr->register_signal_dispatcher([&](int signal) {
        signal_dispatcher(&server, &loop, signal);
    });
    signalfd_ptr->start();

    // server config
    server.set_thread_pool_capacity(thread_pool_capacity);
    server.register_http_request_callback(http_request_callback);
    server.register_connect_success_callback([](const TcpConnectSocketfdPtr
                                                    &tcp_connect_socketfd_ptr) {
        LOG_TRACE
            << "TCP connection established, id: "
            << tcp_connect_socketfd_ptr->get_id()
            << ", path: "
                   + tcp_connect_socketfd_ptr->get_local_address().to_string()
                   + " -> "
                   + tcp_connect_socketfd_ptr->get_remote_address().to_string();
    });
#ifdef __HTTP_EXAMPLE_RUN_BENCHMARK
    server.set_connection_timeout_interval(
        xubinh_server::util::TimeInterval::FOREVER
    ); // FOREVER = don't start timer for checking inactive connections
#else
    server.set_connection_timeout_interval(
        15 * xubinh_server::util::TimeInterval::SECOND
    ); // 15 sec
#endif
    server.start();
    LOG_INFO << "server started listening on " + server_address.to_string();
    xubinh_server::LogCollector::flush();

    loop.loop();

    LOG_INFO << "test finished";

    return 0;
}