#ifndef SIMPLE_HTTP_LISTENER_H
#define SIMPLE_HTTP_LISTENER_H

#include "IHttpServer.h"

#include <chrono>
#include <functional>
#include <memory>
#include <string>

class SimpleHttpListenerResponseWriter;

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

    ~SimpleHttpListener();

    int runUntilStopped();

private:
    std::string host_;
    int port_;
    IHttpServer& server_;
    std::function<bool()> shouldStop_;
    std::function<void()> onTick_;
    std::chrono::milliseconds clientIoTimeout_;
    std::unique_ptr<SimpleHttpListenerResponseWriter> responseWriter_;

    int createListeningSocket() const;
    bool prepareClientResponse(
        int clientSocket,
        std::string& rawResponse,
        bool& bulkResponse) const;
};

#endif
