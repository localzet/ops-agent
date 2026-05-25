#include "ops_agent/infrastructure/linux/SystemdServiceManager.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <csignal>
#include <cstring>
#include <map>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <string>

#if defined(__linux__)
#include <fcntl.h>
#include <poll.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

namespace ops_agent::infrastructure::linux {
namespace {

bool isValidUnitName(const std::string& name)
{
    static const std::regex pattern{"^[A-Za-z0-9_.@:-]+$"};
    return !name.empty() && name.size() <= 128 && std::regex_match(name, pattern);
}

[[maybe_unused]] std::string normalizedStatus(const features::services::ServiceStatusDto& dto)
{
    if (dto.load_state == "not-found") {
        return "not-found";
    }
    if (dto.active_state == "failed") {
        return "failed";
    }
    if (dto.active_state == "active") {
        return "active";
    }
    return "inactive";
}

[[maybe_unused]] std::map<std::string, std::string> parseProperties(const std::string& output)
{
    std::map<std::string, std::string> properties;
    std::istringstream stream{output};
    std::string line;

    while (std::getline(stream, line)) {
        const auto delimiter = line.find('=');
        if (delimiter == std::string::npos) {
            continue;
        }
        properties.emplace(line.substr(0, delimiter), line.substr(delimiter + 1));
    }

    return properties;
}

#if defined(__linux__)
struct CommandResult {
    int exit_code{-1};
    bool timed_out{false};
    std::string stdout_text;
    std::string stderr_text;
};

void setNonBlocking(int fd)
{
    const int flags = fcntl(fd, F_GETFL, 0);
    if (flags >= 0) {
        (void)fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    }
}

void appendAvailable(int fd, std::string& output)
{
    std::array<char, 4096> buffer{};
    while (true) {
        const ssize_t bytes_read = read(fd, buffer.data(), buffer.size());
        if (bytes_read > 0) {
            output.append(buffer.data(), static_cast<std::size_t>(bytes_read));
            continue;
        }
        if (bytes_read == 0 || (bytes_read < 0 && errno != EINTR)) {
            break;
        }
    }
}

CommandResult runSystemctlShow(const std::string& name, int timeout_ms)
{
    int stdout_pipe[2]{};
    int stderr_pipe[2]{};
    if (pipe(stdout_pipe) != 0 || pipe(stderr_pipe) != 0) {
        throw std::runtime_error("failed to create subprocess pipe");
    }

    const pid_t pid = fork();
    if (pid < 0) {
        close(stdout_pipe[0]);
        close(stdout_pipe[1]);
        close(stderr_pipe[0]);
        close(stderr_pipe[1]);
        throw std::runtime_error("failed to fork systemctl process");
    }

    if (pid == 0) {
        dup2(stdout_pipe[1], STDOUT_FILENO);
        dup2(stderr_pipe[1], STDERR_FILENO);
        close(stdout_pipe[0]);
        close(stdout_pipe[1]);
        close(stderr_pipe[0]);
        close(stderr_pipe[1]);

        const std::string unit = name.find(".service") == std::string::npos ? name + ".service" : name;
        char command[] = "systemctl";
        char arg_show[] = "show";
        char arg_no_pager[] = "--no-pager";
        char arg_load[] = "--property=LoadState";
        char arg_active[] = "--property=ActiveState";
        char arg_sub[] = "--property=SubState";
        char arg_description[] = "--property=Description";
        char* const argv[] = {
            command,
            arg_show,
            arg_no_pager,
            arg_load,
            arg_active,
            arg_sub,
            arg_description,
            const_cast<char*>(unit.c_str()),
            nullptr,
        };
        execvp(command, argv);
        _exit(127);
    }

    close(stdout_pipe[1]);
    close(stderr_pipe[1]);
    setNonBlocking(stdout_pipe[0]);
    setNonBlocking(stderr_pipe[0]);

    CommandResult result;
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout_ms);

    while (true) {
        appendAvailable(stdout_pipe[0], result.stdout_text);
        appendAvailable(stderr_pipe[0], result.stderr_text);

        int status = 0;
        const pid_t wait_status = waitpid(pid, &status, WNOHANG);
        if (wait_status == pid) {
            if (WIFEXITED(status)) {
                result.exit_code = WEXITSTATUS(status);
            } else if (WIFSIGNALED(status)) {
                result.exit_code = 128 + WTERMSIG(status);
            }
            break;
        }

        if (std::chrono::steady_clock::now() >= deadline) {
            result.timed_out = true;
            kill(pid, SIGKILL);
            (void)waitpid(pid, &status, 0);
            break;
        }

        pollfd fds[2]{{stdout_pipe[0], POLLIN, 0}, {stderr_pipe[0], POLLIN, 0}};
        (void)poll(fds, 2, 25);
    }

    appendAvailable(stdout_pipe[0], result.stdout_text);
    appendAvailable(stderr_pipe[0], result.stderr_text);
    close(stdout_pipe[0]);
    close(stderr_pipe[0]);

    return result;
}
#endif

} // namespace

SystemdServiceManager::SystemdServiceManager(int timeout_ms)
    : timeout_ms_(std::clamp(timeout_ms, 100, 5000))
{
}

features::services::ServiceStatusDto SystemdServiceManager::status(const std::string& name) const
{
    features::services::ServiceStatusDto dto;
    dto.name = name;

    if (!isValidUnitName(name)) {
        dto.status = "not-found";
        dto.error = "invalid service name";
        return dto;
    }

#if !defined(__linux__)
    dto.status = "not-found";
    dto.error = "unsupported platform";
    return dto;
#else
    const auto command = runSystemctlShow(name, timeout_ms_);
    if (command.timed_out) {
        dto.status = "not-found";
        dto.error = "systemctl timeout";
        return dto;
    }

    const auto properties = parseProperties(command.stdout_text);
    auto property = [&properties](const std::string& key) {
        const auto it = properties.find(key);
        return it == properties.end() ? std::string{} : it->second;
    };

    dto.load_state = property("LoadState");
    dto.active_state = property("ActiveState");
    dto.sub_state = property("SubState");
    dto.description = property("Description");

    if (dto.load_state.empty()) {
        dto.load_state = "not-found";
    }
    if (dto.active_state.empty()) {
        dto.active_state = dto.load_state == "not-found" ? "inactive" : "unknown";
    }
    if (dto.sub_state.empty()) {
        dto.sub_state = "unknown";
    }

    dto.status = normalizedStatus(dto);
    if (command.exit_code != 0 && dto.status != "not-found") {
        dto.error = command.stderr_text.empty() ? "systemctl returned non-zero exit code" : command.stderr_text;
    }
    return dto;
#endif
}

} // namespace ops_agent::infrastructure::linux
