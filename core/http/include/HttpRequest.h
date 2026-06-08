#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H

#include <map>
#include <string>

struct HttpRequest {
    std::string method;
    std::string url;
    std::map<std::string, std::string> headers;
    std::string body;
};

#endif
