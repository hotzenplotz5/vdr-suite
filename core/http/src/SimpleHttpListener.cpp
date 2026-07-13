#include "SimpleHttpListener.h"

#include "HttpServerRequest.h"
#include "HttpServerResponse.h"

#include <cerrno>
#include <cctype>
#include <chrono>
#include <cstring>
#include <iostream>
#include <netdb.h>
#include <sstream>
#include <string>
#include <cstdlib>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>
#include <utility>

namespace {

constexpr auto DEFAULT_CLIENT_IO_TIMEOUT = std::chrono::seconds(5);

timeval socketTimeoutValue(std::chrono::milliseconds timeout)
{
    if (timeout.count() <= 0) {
        timeout = std::chrono::milliseconds(1);
    }

    const auto totalMicroseconds =
        std::chrono::duration_cast<std::chrono::microseconds>(timeout).count();

    timeval value{};
    value.tv_sec = static_cast<time_t>(totalMicroseconds / 1000000);
    value.tv_usec = static_cast<suseconds_t>(totalMicroseconds % 1000000);
    return value;
}

void configureClientSocketTimeouts(
    int socketFd,
    std::chrono::milliseconds timeout)
{
    const timeval value = socketTimeoutValue(timeout);

    if (setsockopt(
            socketFd,
            SOL_SOCKET,
            SO_RCVTIMEO,
            &value,
            sizeof(value)) != 0) {
        std::cerr
            << "failed to configure HTTP client receive timeout: "
            << std::strerror(errno)
            << std::endl;
    }

    if (setsockopt(
            socketFd,
            SOL_SOCKET,
            SO_SNDTIMEO,
            &value,
            sizeof(value)) != 0) {
        std::cerr
            << "failed to configure HTTP client send timeout: "
            << std::strerror(errno)
            << std::endl;
    }
}

int noSignalSendFlags()
{
#ifdef MSG_NOSIGNAL
    return MSG_NOSIGNAL;
#else
    return 0;
#endif
}

std::string lowerAscii(const std::string& value)
{
    std::string lowered;
    lowered.reserve(value.size());

    for (const char character : value) {
        lowered.push_back(
            static_cast<char>(
                std::tolower(
                    static_cast<unsigned char>(character))));
    }

    return lowered;
}

bool hasHeader(
    const HttpServerResponse& response,
    const std::string& headerName)
{
    const std::string wanted = lowerAscii(headerName);

    for (const auto& header : response.headers) {
        if (lowerAscii(header.first) == wanted) {
            return true;
        }
    }

    return false;
}

std::string serializedHeaderValue(
    const std::string& headerName,
    const std::string& headerValue)
{
    if (lowerAscii(headerName) == "content-type" &&
        lowerAscii(headerValue) == "application/json") {
        return "application/json; charset=utf-8";
    }

    return headerValue;
}

std::string reasonPhrase(int statusCode)
{
    if (statusCode == 200) {
        return "OK";
    }

    if (statusCode == 204) {
        return "No Content";
    }

    if (statusCode == 401) {
        return "Unauthorized";
    }

    if (statusCode == 404) {
        return "Not Found";
    }

    if (statusCode == 405) {
        return "Method Not Allowed";
    }

    if (statusCode == 503) {
        return "Service Unavailable";
    }

    return "OK";
}

HttpServerRequest parseRequest(const std::string& rawRequest)
{
    HttpServerRequest request;

    const std::size_t headerEnd = rawRequest.find("\r\n\r\n");

    std::string headerBlock = rawRequest;
    if (headerEnd != std::string::npos) {
        headerBlock = rawRequest.substr(0, headerEnd);
        request.body = rawRequest.substr(headerEnd + 4);
    }

    std::istringstream stream(headerBlock);
    std::string requestLine;

    if (!std::getline(stream, requestLine)) {
        return request;
    }

    if (!requestLine.empty() && requestLine.back() == '\r') {
        requestLine.pop_back();
    }

    std::istringstream requestLineStream(requestLine);
    std::string httpVersion;

    requestLineStream
        >> request.method
        >> request.path
        >> httpVersion;

    std::string line;
    while (std::getline(stream, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        if (line.empty()) {
            break;
        }

        const std::size_t colon = line.find(':');
        if (colon == std::string::npos) {
            continue;
        }

        std::string key = line.substr(0, colon);
        std::string value = line.substr(colon + 1);

        while (!value.empty() && value.front() == ' ') {
            value.erase(value.begin());
        }

        request.headers[key] = value;
    }

    return request;
}

std::string serializeResponse(const HttpServerResponse& response)
{
    std::ostringstream stream;

    stream
        << "HTTP/1.1 "
        << response.statusCode
        << " "
        << reasonPhrase(response.statusCode)
        << "\r\n";

    for (const auto& header : response.headers) {
        stream
            << header.first
            << ": "
            << serializedHeaderValue(header.first, header.second)
            << "\r\n";
    }

    if (!hasHeader(response, "Cache-Control")) {
        stream << "Cache-Control: no-cache\r\n";
    }

    if (!hasHeader(response, "X-Content-Type-Options")) {
        stream << "X-Content-Type-Options: nosniff\r\n";
    }

    stream
        << "Content-Length: "
        << response.body.size()
        << "\r\n";

    stream << "Connection: close\r\n";
    stream << "\r\n";
    stream << response.body;

    return stream.str();
}

bool hasCompleteHeaders(const std::string& rawRequest)
{
    return rawRequest.find("\r\n\r\n") != std::string::npos;
}

std::size_t contentLength(const HttpServerRequest& request)
{
    auto iterator = request.headers.find("Content-Length");
    if (iterator == request.headers.end()) {
        return 0;
    }

    char* end = nullptr;
    const unsigned long value =
        std::strtoul(iterator->second.c_str(), &end, 10);

    if (end == iterator->second.c_str()) {
        return 0;
    }

    return static_cast<std::size_t>(value);
}

bool hasCompleteBody(
    const HttpServerRequest& request,
    const std::size_t expectedContentLength)
{
    return request.body.size() >= expectedContentLength;
}

void writeAll(int socketFd, const std::string& data)
{
    std::size_t offset = 0;

    while (offset < data.size()) {
        const ssize_t written = send(
            socketFd,
            data.data() + offset,
            data.size() - offset,
            noSignalSendFlags());

        if (written <= 0) {
            return;
        }

        offset += static_cast<std::size_t>(written);
    }
}

} // namespace

SimpleHttpListener::SimpleHttpListener(
    std::string host,
    int port,
    IHttpServer& server)
    : SimpleHttpListener(
          std::move(host),
          port,
          server,
          []() { return false; },
          []() {})
{
}

SimpleHttpListener::SimpleHttpListener(
    std::string host,
    int port,
    IHttpServer& server,
    std::function<bool()> shouldStop)
    : SimpleHttpListener(
          std::move(host),
          port,
          server,
          std::move(shouldStop),
          []() {})
{
}

SimpleHttpListener::SimpleHttpListener(
    std::string host,
    int port,
    IHttpServer& server,
    std::function<bool()> shouldStop,
    std::function<void()> onTick)
    : SimpleHttpListener(
          std::move(host),
          port,
          server,
          std::move(shouldStop),
          std::move(onTick),
          DEFAULT_CLIENT_IO_TIMEOUT)
{
}

SimpleHttpListener::SimpleHttpListener(
    std::string host,
    int port,
    IHttpServer& server,
    std::function<bool()> shouldStop,
    std::function<void()> onTick,
    std::chrono::milliseconds clientIoTimeout)
    : host_(std::move(host)),
      port_(port),
      server_(server),
      shouldStop_(std::move(shouldStop)),
      onTick_(std::move(onTick)),
      clientIoTimeout_(clientIoTimeout)
{
}

int SimpleHttpListener::runUntilStopped()
{
    const int listenSocket = createListeningSocket();

    if (listenSocket < 0) {
        return 1;
    }

    std::cout
        << "simple HTTP listener running on "
        << host_
        << ":"
        << port_
        << std::endl;

    while (!shouldStop_()) {
        if (onTick_) {
            onTick_();
        }

        fd_set readSet;
        FD_ZERO(&readSet);
        FD_SET(listenSocket, &readSet);

        timeval timeout{};
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;

        const int ready = select(
            listenSocket + 1,
            &readSet,
            nullptr,
            nullptr,
            &timeout);

        if (ready < 0) {
            if (errno == EINTR) {
                break;
            }

            std::cerr
                << "select failed: "
                << std::strerror(errno)
                << std::endl;

            continue;
        }

        if (ready == 0) {
            continue;
        }

        const int clientSocket = accept(
            listenSocket,
            nullptr,
            nullptr);

        if (clientSocket < 0) {
            if (errno == EINTR) {
                break;
            }

            std::cerr
                << "accept failed: "
                << std::strerror(errno)
                << std::endl;

            continue;
        }

        configureClientSocketTimeouts(
            clientSocket,
            clientIoTimeout_);
        handleClient(clientSocket);
        close(clientSocket);
    }

    close(listenSocket);

    return 0;
}

int SimpleHttpListener::createListeningSocket() const
{
    addrinfo hints{};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    addrinfo* result = nullptr;
    const std::string port = std::to_string(port_);

    const int lookup = getaddrinfo(
        host_.c_str(),
        port.c_str(),
        &hints,
        &result);

    if (lookup != 0) {
        std::cerr
            << "getaddrinfo failed: "
            << gai_strerror(lookup)
            << std::endl;
        return -1;
    }

    int listenSocket = -1;

    for (addrinfo* current = result; current != nullptr; current = current->ai_next) {
        listenSocket = socket(
            current->ai_family,
            current->ai_socktype,
            current->ai_protocol);

        if (listenSocket < 0) {
            continue;
        }

        int reuseAddress = 1;
        setsockopt(
            listenSocket,
            SOL_SOCKET,
            SO_REUSEADDR,
            &reuseAddress,
            sizeof(reuseAddress));

        if (bind(listenSocket, current->ai_addr, current->ai_addrlen) == 0) {
            break;
        }

        close(listenSocket);
        listenSocket = -1;
    }

    freeaddrinfo(result);

    if (listenSocket < 0) {
        std::cerr
            << "failed to bind HTTP listener to "
            << host_
            << ":"
            << port
            << std::endl;
        return -1;
    }

    if (listen(listenSocket, 16) != 0) {
        std::cerr
            << "listen failed: "
            << std::strerror(errno)
            << std::endl;
        close(listenSocket);
        return -1;
    }

    return listenSocket;
}

void SimpleHttpListener::handleClient(int clientSocket) const
{
    std::string rawRequest;
    char buffer[4096];

    while (!hasCompleteHeaders(rawRequest)) {
        const ssize_t received = recv(
            clientSocket,
            buffer,
            sizeof(buffer),
            0);

        if (received <= 0) {
            return;
        }

        rawRequest.append(buffer, static_cast<std::size_t>(received));
    }

    HttpServerRequest request = parseRequest(rawRequest);
    const std::size_t expectedContentLength =
        contentLength(request);

    while (!hasCompleteBody(request, expectedContentLength)) {
        const ssize_t received = recv(
            clientSocket,
            buffer,
            sizeof(buffer),
            0);

        if (received <= 0) {
            return;
        }

        rawRequest.append(buffer, static_cast<std::size_t>(received));
        request = parseRequest(rawRequest);
    }

    const HttpServerResponse response = server_.handleRequest(request);
    const std::string rawResponse = serializeResponse(response);

    writeAll(clientSocket, rawResponse);
}
