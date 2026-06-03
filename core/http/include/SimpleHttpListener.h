#ifndef SIMPLE_HTTP_LISTENER_H
#define SIMPLE_HTTP_LISTENER_H

#include "IHttpServer.h"

#include <string>

class SimpleHttpListener {
public:
    SimpleHttpListener(
        std::string host,
        int port,
        IHttpServer& server);

    int runUntilStopped();

private:
    std::string host_;
    int port_;
    IHttpServer& server_;

    int createListeningSocket() const;
    void handleClient(int clientSocket) const;
};

#endif
