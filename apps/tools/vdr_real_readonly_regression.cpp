#include "BasicHttpClient.h"
#include "HttpRequest.h"
#include "HttpResponse.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

struct Endpoint {
    std::string name;
    std::string path;
};

std::string envOrDefault(const char* name, const std::string& fallback)
{
    const char* value = std::getenv(name);
    if (value == nullptr || std::string(value).empty()) {
        return fallback;
    }
    return value;
}

int envPort()
{
    const char* value = std::getenv("VDR_SUITE_RESTFULAPI_PORT");
    if (value == nullptr || std::string(value).empty()) {
        return 8002;
    }
    return std::stoi(value);
}

HttpResponse get(BasicHttpClient& client, const std::string& path)
{
    HttpRequest request;
    request.method = "GET";
    request.url = path;
    request.headers["Accept"] = "application/json";
    return client.execute(request);
}

bool jsonLike(const std::string& body)
{
    return body.find('{') != std::string::npos ||
           body.find('[') != std::string::npos;
}

void printHelp()
{
    std::cout
        << "Real VDR read-only regression helper\n\n"
        << "This tool talks to a real VDR RESTfulAPI instance.\n"
        << "It only performs GET requests and never modifies the VDR.\n\n"
        << "Environment:\n"
        << "  VDR_SUITE_RESTFULAPI_HOST   default: 127.0.0.1\n"
        << "  VDR_SUITE_RESTFULAPI_PORT   default: 8002\n\n"
        << "Usage:\n"
        << "  /tmp/vdr_suite_real_readonly_regression --help\n"
        << "  /tmp/vdr_suite_real_readonly_regression --run\n";
}

int main(int argc, char** argv)
{
    if (argc < 2 || std::string(argv[1]) == "--help") {
        printHelp();
        return 0;
    }

    if (std::string(argv[1]) != "--run") {
        std::cerr << "Use --run to execute the read-only regression helper." << std::endl;
        return 1;
    }

    const std::string host =
        envOrDefault("VDR_SUITE_RESTFULAPI_HOST", "127.0.0.1");
    const int port =
        envPort();

    const std::vector<Endpoint> endpoints = {
        {"INFO", "/info.json"},
        {"CHANNELS", "/channels.json"},
        {"EVENTS", "/events.json"},
        {"RECORDINGS", "/recordings.json"},
        {"TIMERS", "/timers.json"},
        {"SEARCHTIMERS", "/searchtimers.json"}
    };

    BasicHttpClient client(host, port);

    bool success = true;

    std::cout << "Real VDR Read-only Regression Test\n";
    std::cout << "host=" << host << "\n";
    std::cout << "port=" << port << "\n\n";

    for (const Endpoint& endpoint : endpoints) {
        const HttpResponse response =
            get(client, endpoint.path);

        const bool passed =
            response.statusCode >= 200 &&
            response.statusCode < 300 &&
            jsonLike(response.body);

        success = success && passed;

        std::cout
            << (passed ? "PASS " : "FAIL ")
            << endpoint.name
            << " - status=" << response.statusCode
            << " bodyBytes=" << response.body.size()
            << " path=" << endpoint.path
            << "\n";
    }

    std::cout << "\nResult: "
              << (success ? "SUCCESS" : "FAILURE")
              << std::endl;

    return success ? 0 : 2;
}
