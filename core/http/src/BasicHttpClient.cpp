#include "BasicHttpClient.h"

#include <chrono>
#include <cerrno>
#include <cstring>
#include <netdb.h>
#include <sstream>
#include <stdexcept>
#include <string>
#include <sys/socket.h>
#include <unistd.h>

namespace {

std::string buildHttpRequest(const std::string& host, int port, const HttpRequest& request)
{
    std::ostringstream stream;

    stream << request.method << " " << request.url << " HTTP/1.0\r\n";
    stream << "Host: " << host << ":" << port << "\r\n";
    stream << "Connection: close\r\n";

    for (const auto& header : request.headers) {
        stream << header.first << ": " << header.second << "\r\n";
    }

    if (!request.body.empty()) {
        stream << "Content-Length: " << request.body.size() << "\r\n";
    }

    stream << "\r\n";
    stream << request.body;

    return stream.str();
}

HttpResponse parseHttpResponse(const std::string& raw)
{
    HttpResponse response;

    const std::string separator = "\r\n\r\n";
    const std::size_t bodyStart = raw.find(separator);

    const std::string headerBlock = bodyStart == std::string::npos
        ? raw
        : raw.substr(0, bodyStart);

    response.body = bodyStart == std::string::npos
        ? std::string()
        : raw.substr(bodyStart + separator.size());

    std::istringstream headerStream(headerBlock);
    std::string statusLine;

    if (std::getline(headerStream, statusLine)) {
        if (!statusLine.empty() && statusLine.back() == '\r') {
            statusLine.pop_back();
        }

        std::istringstream statusStream(statusLine);
        std::string httpVersion;
        statusStream >> httpVersion >> response.statusCode;
    }

    std::string line;
    while (std::getline(headerStream, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
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

        response.headers[key] = value;
    }

    return response;
}

} // namespace

BasicHttpClient::BasicHttpClient(std::string host, int port, IRuntimeLogger* logger)
    : host_(std::move(host)),
      port_(port),
      logger_(logger)
{
}

void BasicHttpClient::log(RuntimeLogLevel level, const std::string& message) const
{
    if (logger_ == nullptr) {
        return;
    }

    logger_->write(RuntimeLogEntry{level, "BasicHttpClient", message});
}

HttpResponse BasicHttpClient::execute(const HttpRequest& request) const
{
    const auto started = std::chrono::steady_clock::now();
    addrinfo hints{};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    addrinfo* result = nullptr;
    const std::string port = std::to_string(port_);

    const int lookup = getaddrinfo(host_.c_str(), port.c_str(), &hints, &result);
    if (lookup != 0) {
        throw std::runtime_error(std::string("getaddrinfo failed: ") + gai_strerror(lookup));
    }

    int socketFd = -1;

    for (addrinfo* current = result; current != nullptr; current = current->ai_next) {
        socketFd = socket(current->ai_family, current->ai_socktype, current->ai_protocol);
        if (socketFd == -1) {
            continue;
        }

        if (connect(socketFd, current->ai_addr, current->ai_addrlen) == 0) {
            break;
        }

        close(socketFd);
        socketFd = -1;
    }

    freeaddrinfo(result);

    if (socketFd == -1) {
        throw std::runtime_error("connect failed to " + host_ + ":" + port);
    }

    const std::string rawRequest = buildHttpRequest(host_, port_, request);
    std::size_t bytesSent = 0;

    while (bytesSent < rawRequest.size()) {
        const ssize_t written = send(
            socketFd,
            rawRequest.data() + bytesSent,
            rawRequest.size() - bytesSent,
            0);

        if (written <= 0) {
            close(socketFd);
            throw std::runtime_error("send failed: " + std::string(std::strerror(errno)));
        }

        bytesSent += static_cast<std::size_t>(written);
    }

    std::string rawResponse;
    char buffer[4096];

    while (true) {
        const ssize_t received = recv(socketFd, buffer, sizeof(buffer), 0);

        if (received < 0) {
            close(socketFd);
            throw std::runtime_error("recv failed: " + std::string(std::strerror(errno)));
        }

        if (received == 0) {
            break;
        }

        rawResponse.append(buffer, static_cast<std::size_t>(received));
    }

    close(socketFd);

    HttpResponse response = parseHttpResponse(rawResponse);

    const auto finished = std::chrono::steady_clock::now();
    const auto durationMs = std::chrono::duration_cast<std::chrono::milliseconds>(finished - started).count();
    log(
        RuntimeLogLevel::Info,
        request.method + " " + request.url + " finished with status " +
            std::to_string(response.statusCode) + " in " +
            std::to_string(durationMs) + " ms, body " +
            std::to_string(response.body.size()) + " bytes");

    return response;
}
