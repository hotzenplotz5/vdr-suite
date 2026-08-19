#ifndef SIMPLE_HTTP_LISTENER_H
#define SIMPLE_HTTP_LISTENER_H

#include "IHttpServer.h"

#include <chrono>
#include <functional>
#include <string>

class SimpleHttpListener {
public:
    SimpleHttpListener(
        std::string host,
        int port,
        IHttpServer& server);

    SimpleHttpListener(
        std::string host,
        int port,
        IHttpServer& server,
        std::function<bool()> shouldStop);

    SimpleHttpListener(
        std::string host,
        int port,
        IHttpServer& server,
        std::function<bool()> shouldStop,
        std::function<void()> onTick);

    SimpleHttpListener(
        std::string host,
        int port,
        IHttpServer& server,
        std::function<bool()> shouldStop,
        std::function<void()> onTick,
        std::chrono::milliseconds clientIoTimeout);

    int runUntilStopped();

private:
    std::string host_;
    int port_;
    IHttpServer& server_;
    std::function<bool()> shouldStop_;
    std::function<void()> onTick_;
    std::chrono::milliseconds clientIoTimeout_;

    int createListeningSocket() const;
    bool prepareClientResponse(
        int clientSocket,
        std::string& rawResponse,
        bool& imageResponse,
        std::string& streamBodyPath) const;
};

#endif
