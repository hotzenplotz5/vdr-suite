#ifndef SIMPLE_HTTP_LISTENER_H
#define SIMPLE_HTTP_LISTENER_H

#include "IHttpServer.h"

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

    int runUntilStopped();

private:
    std::string host_;
    int port_;
    IHttpServer& server_;
    std::function<bool()> shouldStop_;

    int createListeningSocket() const;
    void handleClient(int clientSocket) const;
};

#endif
