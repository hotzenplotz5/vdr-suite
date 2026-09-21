#include "RestfulApiEventStreamClient.h"

#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <netdb.h>
#include <sstream>
#include <sys/socket.h>
#include <unistd.h>

#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif

RestfulApiEventStreamClient::RestfulApiEventStreamClient(
    std::string backendId,
    std::string host,
    int port,
    std::function<void(const std::string& backendId)> onChangeHint)
    : backendId_(std::move(backendId)),
      host_(std::move(host)),
      port_(port),
      onChangeHint_(std::move(onChangeHint)),
      running_(false)
{
}

RestfulApiEventStreamClient::~RestfulApiEventStreamClient()
{
    stop();
}

void RestfulApiEventStreamClient::start()
{
    if (running_.exchange(true)) {
        return;
    }

    thread_ = std::thread(&RestfulApiEventStreamClient::runLoop, this);
}

void RestfulApiEventStreamClient::stop()
{
    if (!running_.exchange(false)) {
        return;
    }

    if (thread_.joinable()) {
        thread_.join();
    }
}

bool RestfulApiEventStreamClient::running() const
{
    return running_.load();
}

void RestfulApiEventStreamClient::runLoop()
{
    std::cout
        << "restfulapi event stream client starting for backend "
        << backendId_
        << " at "
        << host_
        << ":"
        << port_
        << std::endl;

    while (running_.load()) {
        if (!connectAndReadOnce()) {
            for (int i = 0; i < 50 && running_.load(); ++i) {
                usleep(100000);
            }
        }
    }

    std::cout
        << "restfulapi event stream client stopped for backend "
        << backendId_
        << std::endl;
}

bool RestfulApiEventStreamClient::connectAndReadOnce()
{
    int socketFd = -1;

    if (!connectSocket(socketFd)) {
        return false;
    }

    const bool requestSent = sendRequest(socketFd);
    const bool readOk = requestSent && readLoop(socketFd);

    close(socketFd);
    return readOk;
}

bool RestfulApiEventStreamClient::connectSocket(int& socketFd)
{
    socketFd = -1;

    addrinfo hints;
    std::memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    const std::string portString = std::to_string(port_);

    addrinfo* result = nullptr;
    const int lookupResult =
        getaddrinfo(host_.c_str(), portString.c_str(), &hints, &result);

    if (lookupResult != 0) {
        return false;
    }

    for (addrinfo* entry = result; entry != nullptr; entry = entry->ai_next) {
        socketFd = socket(entry->ai_family, entry->ai_socktype, entry->ai_protocol);

        if (socketFd < 0) {
            continue;
        }

        if (connect(socketFd, entry->ai_addr, entry->ai_addrlen) == 0) {
            freeaddrinfo(result);
            return true;
        }

        close(socketFd);
        socketFd = -1;
    }

    freeaddrinfo(result);
    return false;
}

bool RestfulApiEventStreamClient::sendRequest(int socketFd)
{
    std::ostringstream request;
    request
        << "GET /eventstream HTTP/1.1\r\n"
        << "Host: " << host_ << ":" << port_ << "\r\n"
        << "Accept: text/event-stream\r\n"
        << "Connection: keep-alive\r\n"
        << "\r\n";

    const std::string payload = request.str();
    const char* cursor = payload.c_str();
    std::size_t remaining = payload.size();

    while (remaining > 0) {
        const ssize_t sent = send(socketFd, cursor, remaining, MSG_NOSIGNAL);

        if (sent <= 0) {
            return false;
        }

        cursor += sent;
        remaining -= static_cast<std::size_t>(sent);
    }

    return true;
}

bool RestfulApiEventStreamClient::readLoop(int socketFd)
{
    std::string buffer;
    char chunk[1024];

    while (running_.load()) {
        const ssize_t received = recv(socketFd, chunk, sizeof(chunk), 0);

        if (received <= 0) {
            return false;
        }

        buffer.append(chunk, static_cast<std::size_t>(received));

        std::size_t headerEnd = buffer.find("\r\n\r\n");
        if (headerEnd != std::string::npos) {
            buffer.erase(0, headerEnd + 4);
        }

        while (true) {
            std::size_t blockEnd = buffer.find("\n\n");

            if (blockEnd == std::string::npos) {
                break;
            }

            const std::string block = buffer.substr(0, blockEnd);
            buffer.erase(0, blockEnd + 2);

            handleSseBlock(block);
        }
    }

    return true;
}

void RestfulApiEventStreamClient::handleSseBlock(const std::string& block)
{
    if (!blockIsChangeEvent(block)) {
        return;
    }

    std::cout
        << "restfulapi event stream change hint for backend "
        << backendId_
        << std::endl;

    if (onChangeHint_) {
        onChangeHint_(backendId_);
    }
}

bool RestfulApiEventStreamClient::blockIsChangeEvent(const std::string& block) const
{
    return block.find("event: vdr-change") != std::string::npos &&
        block.find("\"domains\"") != std::string::npos;
}
