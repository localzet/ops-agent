#include "ops_agent/infrastructure/linux/SocketTcpClient.hpp"

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <cstring>
#include <string>

#if defined(__linux__)
#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <poll.h>
#include <signal.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

namespace ops_agent::infrastructure::linux {
namespace {

#if defined(__linux__)
class FdGuard {
public:
    explicit FdGuard(int fd)
        : fd_(fd)
    {
    }

    ~FdGuard()
    {
        if (fd_ >= 0) {
            close(fd_);
        }
    }

    int get() const
    {
        return fd_;
    }

private:
    int fd_;
};

int remainingMilliseconds(std::chrono::steady_clock::time_point deadline)
{
    const auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(
        deadline - std::chrono::steady_clock::now());
    return std::max(0, static_cast<int>(remaining.count()));
}

addrinfo* resolveWithTimeout(const std::string& host,
                             const std::string& port,
                             const addrinfo& hints,
                             int timeout_ms,
                             std::string& error)
{
    gaicb request{};
    request.ar_name = host.c_str();
    request.ar_service = port.c_str();
    request.ar_request = &hints;

    gaicb* requests[] = {&request};
    const int start_status = getaddrinfo_a(GAI_NOWAIT, requests, 1, nullptr);
    if (start_status != 0) {
        error = gai_strerror(start_status);
        return nullptr;
    }

    timespec timeout{};
    timeout.tv_sec = timeout_ms / 1000;
    timeout.tv_nsec = static_cast<long>(timeout_ms % 1000) * 1000L * 1000L;

    const int suspend_status = gai_suspend(requests, 1, &timeout);
    if (suspend_status == EAI_AGAIN) {
        (void)gai_cancel(&request);
        error = "dns timeout";
        return nullptr;
    }
    if (suspend_status != 0) {
        (void)gai_cancel(&request);
        error = gai_strerror(suspend_status);
        return nullptr;
    }

    const int resolve_status = gai_error(&request);
    if (resolve_status != 0) {
        error = gai_strerror(resolve_status);
        return nullptr;
    }

    return request.ar_result;
}
#endif

} // namespace

features::tcp_check::TcpCheckDto SocketTcpClient::check(const std::string& host,
                                                        std::uint16_t port,
                                                        int timeout_ms) const
{
    features::tcp_check::TcpCheckDto result;
    result.host = host;
    result.port = port;

#if !defined(__linux__)
    (void)timeout_ms;
    result.error = "unsupported platform";
    return result;
#else
    const auto started_at = std::chrono::steady_clock::now();
    const auto deadline = started_at + std::chrono::milliseconds(std::clamp(timeout_ms, 1, 5000));

    addrinfo hints{};
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_family = AF_UNSPEC;

    const auto port_text = std::to_string(port);
    addrinfo* resolved = resolveWithTimeout(host, port_text, hints, remainingMilliseconds(deadline), result.error);
    if (resolved == nullptr) {
        result.latency_ms = static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - started_at).count());
        return result;
    }

    for (addrinfo* item = resolved; item != nullptr; item = item->ai_next) {
        const int remaining_ms = remainingMilliseconds(deadline);
        if (remaining_ms <= 0) {
            result.error = "timeout";
            break;
        }

        const int fd = socket(item->ai_family, item->ai_socktype, item->ai_protocol);
        if (fd < 0) {
            continue;
        }
        FdGuard guard{fd};

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
            const int poll_status = poll(&pfd, 1, remaining_ms);
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
