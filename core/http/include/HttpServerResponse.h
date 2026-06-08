#ifndef HTTP_SERVER_RESPONSE_H
#define HTTP_SERVER_RESPONSE_H

#include <map>
#include <string>

struct HttpServerResponse {
    int statusCode = 200;
    std::map<std::string, std::string> headers;
    std::string body;
};

#endif
