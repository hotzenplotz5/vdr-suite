#ifndef HTTP_SERVER_RESPONSE_H
#define HTTP_SERVER_RESPONSE_H

#include <map>
#include <string>

struct HttpServerResponse {
    int statusCode = 200;
    std::map<std::string, std::string> headers;
    std::string body;

    // When non-empty the listener sends headers without Content-Length and
    // streams this already-authorized local FIFO/file until EOF or disconnect.
    // The path is never serialized into the HTTP response.
    std::string streamBodyPath;
};

#endif
