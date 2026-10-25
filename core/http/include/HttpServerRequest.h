#ifndef HTTP_SERVER_REQUEST_H
#define HTTP_SERVER_REQUEST_H

#include <map>
#include <string>

struct HttpServerRequest {
    std::string method;
    std::string path;
    std::map<std::string, std::string> headers;
    std::string body;
    // Set by the TCP listener from getpeername(), never parsed from HTTP headers.
    bool peerIsTrustedLan = false;
    // Kernel-derived direct non-loopback LAN peer, never an HTTP header.
    bool peerIsDirectPrivateLan = false;
};

#endif
