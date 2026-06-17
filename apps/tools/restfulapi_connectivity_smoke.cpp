#include "BasicHttpClient.h"

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>

namespace
{
int parsePort(
    const char* value)
{
    if (value == nullptr) {
        return 8002;
    }

    return std::stoi(value);
}

std::string readEnv(
    const char* name,
    const std::string& fallback)
{
    const char* value = std::getenv(name);

    if (value == nullptr || std::string(value).empty()) {
        return fallback;
    }

    return value;
}
}

int main()
{
    const std::string host =
        readEnv("VDR_SUITE_RESTFULAPI_HOST", "127.0.0.1");
    const int port =
        parsePort(std::getenv("VDR_SUITE_RESTFULAPI_PORT"));
    const std::string path =
        readEnv("VDR_SUITE_RESTFULAPI_PATH", "/api/info.json");

    try {
        BasicHttpClient client(host, port);

        HttpRequest request;
        request.method = "GET";
        request.url = path;

        const HttpResponse response =
            client.execute(request);

        std::cout << "RESTfulAPI connectivity smoke" << std::endl;
        std::cout << "host=" << host << std::endl;
        std::cout << "port=" << port << std::endl;
        std::cout << "path=" << path << std::endl;
        std::cout << "status=" << response.statusCode << std::endl;
        std::cout << "bodyBytes=" << response.body.size() << std::endl;

        if (!response.body.empty()) {
            std::cout << "bodyPreview=" << response.body.substr(0, 500)
                      << std::endl;
        }

        return response.statusCode >= 200 && response.statusCode < 500
            ? 0
            : 2;
    }
    catch (const std::exception& error) {
        std::cerr << "RESTfulAPI connectivity smoke failed: "
                  << error.what() << std::endl;
        return 1;
    }
}
