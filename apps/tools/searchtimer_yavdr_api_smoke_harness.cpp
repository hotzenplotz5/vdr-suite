#include "IHttpServer.h"
#include "HttpServerRequest.h"
#include "HttpServerResponse.h"
#include "SearchTimerController.h"
#include "SearchTimerResultJsonSerializer.h"
#include "SearchTimerService.h"
#include "SimpleHttpListener.h"

#include <cstdlib>
#include <iostream>
#include <string>

namespace {

std::string requestPathOnly(const std::string& target)
{
    const std::size_t queryStart = target.find('?');

    if (queryStart == std::string::npos)
    {
        return target;
    }

    return target.substr(0, queryStart);
}

HttpServerResponse toHttpServerResponse(const ApiResponse& apiResponse)
{
    HttpServerResponse response;
    response.statusCode = apiResponse.statusCode;
    response.headers["Content-Type"] = apiResponse.contentType;
    response.body = apiResponse.body;

    return response;
}

HttpServerResponse jsonResponse(
    int statusCode,
    const std::string& body)
{
    HttpServerResponse response;
    response.statusCode = statusCode;
    response.headers["Content-Type"] = "application/json";
    response.body = body;

    return response;
}

class SearchTimerYavdrApiSmokeServer final : public IHttpServer
{
public:
    HttpServerResponse handleRequest(
        const HttpServerRequest& request) const override
    {
        if (request.method != "POST")
        {
            return jsonResponse(
                405,
                "{\"error\":\"method not allowed\"}");
        }

        const std::string path = requestPathOnly(request.path);

        if (path != "/api/searchtimers/real-test" &&
            path != "/api/vdr/searchtimers/real-test")
        {
            return jsonResponse(
                404,
                "{\"error\":\"not found\"}");
        }

        return toHttpServerResponse(
            searchTimerController_.realTestSearchTimerWorkflow(
                request.body));
    }

private:
    mutable SearchTimerService searchTimerService_;
    mutable SearchTimerResultJsonSerializer searchTimerJsonSerializer_;
    mutable SearchTimerController searchTimerController_{
        searchTimerService_,
        searchTimerJsonSerializer_};
};

bool hasArgument(
    int argc,
    char** argv,
    const std::string& value)
{
    for (int index = 1; index < argc; ++index)
    {
        if (argv[index] == value)
        {
            return true;
        }
    }

    return false;
}

std::string argumentValue(
    int argc,
    char** argv,
    const std::string& name,
    const std::string& fallback)
{
    for (int index = 1; index + 1 < argc; ++index)
    {
        if (argv[index] == name)
        {
            return argv[index + 1];
        }
    }

    return fallback;
}

int intArgumentValue(
    int argc,
    char** argv,
    const std::string& name,
    int fallback)
{
    const std::string value =
        argumentValue(
            argc,
            argv,
            name,
            "");

    if (value.empty())
    {
        return fallback;
    }

    char* end = nullptr;
    const long parsed = std::strtol(value.c_str(), &end, 10);

    if (end == value.c_str())
    {
        return fallback;
    }

    return static_cast<int>(parsed);
}

void printHelp()
{
    std::cout
        << "SearchTimer yaVDR local VDR-Suite API smoke harness\n"
        << "\n"
        << "This tool starts a local HTTP endpoint for the VDR-Suite\n"
        << "SearchTimer real-test safety route.\n"
        << "\n"
        << "It serves:\n"
        << "  POST /api/searchtimers/real-test\n"
        << "  POST /api/vdr/searchtimers/real-test\n"
        << "\n"
        << "Defaults:\n"
        << "  --host 127.0.0.1\n"
        << "  --port 18080\n"
        << "\n"
        << "Usage:\n"
        << "  /tmp/vdr_suite_searchtimer_yavdr_api_smoke_harness --help\n"
        << "  /tmp/vdr_suite_searchtimer_yavdr_api_smoke_harness --host 127.0.0.1 --port 18080\n"
        << "\n"
        << "Safety:\n"
        << "  The route uses SearchTimerController::realTestSearchTimerWorkflow().\n"
        << "  Backend mutation remains blocked by the production policy gate.\n";
}

}

int main(int argc, char** argv)
{
    if (hasArgument(argc, argv, "--help") ||
        hasArgument(argc, argv, "-h"))
    {
        printHelp();
        return 0;
    }

    const std::string host =
        argumentValue(
            argc,
            argv,
            "--host",
            "127.0.0.1");

    const int port =
        intArgumentValue(
            argc,
            argv,
            "--port",
            18080);

    SearchTimerYavdrApiSmokeServer server;
    SimpleHttpListener listener(
        host,
        port,
        server);

    try
    {
        return listener.runUntilStopped();
    }
    catch (const std::exception& error)
    {
        std::cerr
            << "SearchTimer yaVDR API smoke harness failed: "
            << error.what()
            << std::endl;

        return 2;
    }
}
