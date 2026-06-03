#include "BasicHttpClient.h"
#include "HttpRequest.h"
#include "HttpResponse.h"
#include "RestfulApiVdrAdapter.h"
#include "VdrConfig.h"
#include "VdrStatus.h"

#include <cstdlib>
#include <exception>
#include <iostream>
#include <string>

namespace {

std::string previewBody(const std::string& body)
{
    constexpr std::size_t maxPreviewLength = 800;

    if (body.size() <= maxPreviewLength) {
        return body;
    }

    return body.substr(0, maxPreviewLength) + "\n... output truncated ...";
}

int parsePort(const char* value)
{
    return std::stoi(std::string(value));
}

} // namespace

int main(int argc, char** argv)
{
    std::string host = "127.0.0.1";
    int port = 8002;

    if (argc >= 2) {
        host = argv[1];
    }

    if (argc >= 3) {
        port = parsePort(argv[2]);
    }

    try {
        std::cout << "VDR-Suite VDR probe" << std::endl;
        std::cout << "Host: " << host << std::endl;
        std::cout << "Port: " << port << std::endl;
        std::cout << "Endpoint: /info.json" << std::endl;
        std::cout << std::endl;

        BasicHttpClient httpClient(host, port);

        HttpRequest request;
        request.method = "GET";
        request.url = "/info.json";
        request.headers["Accept"] = "application/json";

        HttpResponse response = httpClient.execute(request);

        std::cout << "Raw HTTP status: " << response.statusCode << std::endl;
        std::cout << "Raw body size: " << response.body.size() << " bytes" << std::endl;
        std::cout << std::endl;
        std::cout << "Raw body preview:" << std::endl;
        std::cout << previewBody(response.body) << std::endl;
        std::cout << std::endl;

        VdrConfig config;
        config.enabled = true;
        config.mode = "restfulapi";
        config.host = host;
        config.port = port;

        RestfulApiVdrAdapter adapter(config, httpClient);
        VdrStatus status = adapter.getStatus();

        std::cout << "Parsed VDR status:" << std::endl;
        std::cout << "enabled: " << (status.enabled ? "true" : "false") << std::endl;
        std::cout << "mode: " << status.mode << std::endl;
        std::cout << "host: " << status.host << std::endl;
        std::cout << "port: " << status.port << std::endl;
        std::cout << "state: " << status.state << std::endl;

        return response.statusCode == 200 ? 0 : 2;
    } catch (const std::exception& ex) {
        std::cerr << "vdr-probe failed: " << ex.what() << std::endl;
        return 1;
    }
}
