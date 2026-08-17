#include "MediaProcessRunner.h"

#include <cerrno>
#include <chrono>
#include <csignal>
#include <fcntl.h>
#include <poll.h>
#include <string>
#include <sys/stat.h>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>
#include <vector>

namespace
{

std::vector<char*> mutableArgv(const std::vector<std::string>& argv)
{
    std::vector<char*> pointers;
    pointers.reserve(argv.size() + 1);
    for (const std::string& argument : argv) {
        pointers.push_back(const_cast<char*>(argument.c_str()));
    }
    pointers.push_back(nullptr);
    return pointers;
}

void signalProcessGroup(pid_t pid, int signalNumber)
{
    if (pid <= 0) {
        return;
    }

    if (::kill(-pid, signalNumber) != 0) {
        ::kill(pid, signalNumber);
    }
}

int decodedExitCode(int status)
{
    if (WIFEXITED(status)) {
        return WEXITSTATUS(status);
    }
    if (WIFSIGNALED(status)) {
        return 128 + WTERMSIG(status);
    }
    return -1;
}

} // namespace

bool MediaProcessRunner::validInvocation(
    const std::vector<std::string>& argv,
    const std::string& workingDirectory)
{
    return !argv.empty() &&
        !argv.front().empty() &&
        argv.front().front() == '/' &&
        !workingDirectory.empty() &&
        workingDirectory.front() == '/';
}

MediaProcessCaptureResult MediaProcessRunner::runAndCapture(
    const std::vector<std::string>& argv,
    const std::string& workingDirectory,
    std::chrono::milliseconds timeout,
    std::size_t maximumOutputBytes) const
{
    MediaProcessCaptureResult result;
    if (!validInvocation(argv, workingDirectory) ||
        timeout.count() <= 0 || maximumOutputBytes == 0) {
        result.reasonCode = "invalid_process_invocation";
        return result;
    }

    int pipeFds[2] = {-1, -1};
    if (::pipe2(pipeFds, O_CLOEXEC) != 0) {
        result.reasonCode = "process_pipe_failed";
        return result;
    }

    const int readFlags = ::fcntl(pipeFds[0], F_GETFL, 0);
    if (readFlags < 0 || ::fcntl(pipeFds[0], F_SETFL, readFlags | O_NONBLOCK) != 0) {
        ::close(pipeFds[0]);
        ::close(pipeFds[1]);
        result.reasonCode = "process_pipe_failed";
        return result;
    }

    const std::vector<char*> arguments = mutableArgv(argv);
    const pid_t pid = ::fork();
    if (pid < 0) {
        ::close(pipeFds[0]);
        ::close(pipeFds[1]);
        result.reasonCode = "process_fork_failed";
        return result;
    }

    if (pid == 0) {
        ::setpgid(0, 0);
        ::close(pipeFds[0]);
        if (::chdir(workingDirectory.c_str()) != 0 ||
            ::dup2(pipeFds[1], STDOUT_FILENO) < 0) {
            _exit(126);
        }

        const int nullFd = ::open("/dev/null", O_WRONLY | O_CLOEXEC);
        if (nullFd >= 0) {
            ::dup2(nullFd, STDERR_FILENO);
            ::close(nullFd);
        }
        ::close(pipeFds[1]);
        ::execv(argv.front().c_str(), arguments.data());
        _exit(127);
    }

    result.started = true;
    ::close(pipeFds[1]);
    ::setpgid(pid, pid);

    const auto deadline = std::chrono::steady_clock::now() + timeout;
    bool childExited = false;
    bool pipeClosed = false;
    int childStatus = 0;

    while (!childExited || !pipeClosed) {
        char buffer[4096];
        while (true) {
            const ssize_t readBytes = ::read(pipeFds[0], buffer, sizeof(buffer));
            if (readBytes > 0) {
                const std::size_t bytes = static_cast<std::size_t>(readBytes);
                if (result.output.size() + bytes > maximumOutputBytes) {
                    result.outputLimitExceeded = true;
                    result.reasonCode = "process_output_limit_exceeded";
                    signalProcessGroup(pid, SIGKILL);
                    break;
                }
                result.output.append(buffer, bytes);
                continue;
            }
            if (readBytes == 0) {
                pipeClosed = true;
            }
            else if (errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR) {
                pipeClosed = true;
            }
            break;
        }

        if (result.outputLimitExceeded) {
            break;
        }

        if (!childExited) {
            const pid_t waited = ::waitpid(pid, &childStatus, WNOHANG);
            if (waited == pid) {
                childExited = true;
            }
            else if (waited < 0 && errno == ECHILD) {
                childExited = true;
            }
        }

        if (childExited && pipeClosed) {
            break;
        }

        if (std::chrono::steady_clock::now() >= deadline) {
            result.timedOut = true;
            result.reasonCode = "process_timeout";
            signalProcessGroup(pid, SIGKILL);
            break;
        }

        pollfd descriptor{};
        descriptor.fd = pipeFds[0];
        descriptor.events = POLLIN | POLLHUP;
        ::poll(&descriptor, 1, 20);
    }

    ::close(pipeFds[0]);

    if (!childExited) {
        while (::waitpid(pid, &childStatus, 0) < 0 && errno == EINTR) {
        }
        childExited = true;
    }

    result.completed = childExited;
    result.exitCode = decodedExitCode(childStatus);
    if (result.reasonCode.empty() && result.exitCode != 0) {
        result.reasonCode = "process_failed";
    }
    result.success =
        result.completed &&
        !result.timedOut &&
        !result.outputLimitExceeded &&
        result.exitCode == 0;
    return result;
}

pid_t MediaProcessRunner::spawnLogged(
    const std::vector<std::string>& argv,
    const std::string& workingDirectory,
    const std::string& logPath) const
{
    if (!validInvocation(argv, workingDirectory) ||
        logPath.empty() || logPath.front() != '/') {
        return -1;
    }

    const std::vector<char*> arguments = mutableArgv(argv);
    const pid_t pid = ::fork();
    if (pid < 0) {
        return -1;
    }

    if (pid == 0) {
        ::setpgid(0, 0);
        if (::chdir(workingDirectory.c_str()) != 0) {
            _exit(126);
        }

        const int nullFd = ::open("/dev/null", O_RDONLY | O_CLOEXEC);
        const int logFd = ::open(
            logPath.c_str(),
            O_WRONLY | O_CREAT | O_APPEND | O_CLOEXEC,
            0600);
        if (nullFd < 0 || logFd < 0 ||
            ::dup2(nullFd, STDIN_FILENO) < 0 ||
            ::dup2(logFd, STDOUT_FILENO) < 0 ||
            ::dup2(logFd, STDERR_FILENO) < 0) {
            _exit(126);
        }
        ::close(nullFd);
        ::close(logFd);
        ::execv(argv.front().c_str(), arguments.data());
        _exit(127);
    }

    ::setpgid(pid, pid);
    return pid;
}

bool MediaProcessRunner::terminateAndWait(
    pid_t pid,
    std::chrono::milliseconds gracePeriod) const
{
    if (pid <= 0 || gracePeriod.count() < 0) {
        return false;
    }

    int status = 0;
    pid_t waited = ::waitpid(pid, &status, WNOHANG);
    if (waited == pid || (waited < 0 && errno == ECHILD)) {
        return true;
    }

    signalProcessGroup(pid, SIGTERM);
    const auto deadline = std::chrono::steady_clock::now() + gracePeriod;

    while (std::chrono::steady_clock::now() < deadline) {
        waited = ::waitpid(pid, &status, WNOHANG);
        if (waited == pid || (waited < 0 && errno == ECHILD)) {
            return true;
        }
        if (waited < 0 && errno != EINTR) {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    signalProcessGroup(pid, SIGKILL);
    while ((waited = ::waitpid(pid, &status, 0)) < 0 && errno == EINTR) {
    }
    return waited == pid || (waited < 0 && errno == ECHILD);
}
