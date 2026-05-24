#include "ops_agent/infrastructure/linux/SocketTcpClient.hpp"

#include <chrono>
#include <cstring>
#include <string>

#if defined(__linux__)
#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

namespace ops_agent::infrastructure::linux {

features::tcp_check::TcpCheckDto SocketTcpClient::check(const std::string& host,
                                                        std::uint16_t port,
                                                        int timeout_ms) const
{
    features::tcp_check::TcpCheckDto result;
    result.host = host;
    result.port = port;

#if !defined(__linux__)
    result.error = "unsupported platform";
    return result;
#else
    const auto started_at = std::chrono::steady_clock::now();

    addrinfo hints{};
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_family = AF_UNSPEC;

    addrinfo* resolved = nullptr;
    const auto port_text = std::to_string(port);
    const int gai_status = getaddrinfo(host.c_str(), port_text.c_str(), &hints, &resolved);
    if (gai_status != 0) {
        result.error = gai_strerror(gai_status);
        return result;
    }

    for (addrinfo* item = resolved; item != nullptr; item = item->ai_next) {
        const int fd = socket(item->ai_family, item->ai_socktype, item->ai_protocol);
        if (fd < 0) {
            continue;
        }

        const int flags = fcntl(fd, F_GETFL, 0);
        if (flags >= 0) {
            (void)fcntl(fd, F_SETFL, flags | O_NONBLOCK);
        }

        const int connect_status = connect(fd, item->ai_addr, item->ai_addrlen);
        if (connect_status == 0) {
            result.reachable = true;
        } else {
            pollfd pfd{};
            pfd.fd = fd;
            pfd.events = POLLOUT;
            const int poll_status = poll(&pfd, 1, timeout_ms);
            if (poll_status > 0) {
                int socket_error = 0;
                socklen_t socket_error_len = sizeof(socket_error);
                if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &socket_error, &socket_error_len) == 0 && socket_error == 0) {
                    result.reachable = true;
                } else if (socket_error != 0) {
                    result.error = std::strerror(socket_error);
                }
            } else if (poll_status == 0) {
                result.error = "timeout";
            } else {
                result.error = std::strerror(errno);
            }
        }

        close(fd);
        if (result.reachable) {
            break;
        }
    }

    freeaddrinfo(resolved);
    result.latency_ms = static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - started_at).count());
    if (!result.reachable && result.error.empty()) {
        result.error = "connection failed";
    }
    return result;
#endif
}

} // namespace ops_agent::infrastructure::linux
