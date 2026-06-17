#include "BasicHttpClient.h"
#include "RecordingActionBackendPolicy.h"
#include "RecordingActionExecutionService.h"
#include "RestfulApiRecordingActionExecutor.h"

#include <arpa/inet.h>
#include <cassert>
#include <cstring>
#include <memory>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

namespace
{
class SingleRequestHttpServer
{
public:
    explicit SingleRequestHttpServer(
        const std::string& responseBody)
        : responseBody_(responseBody)
    {
        serverFd_ = socket(AF_INET, SOCK_STREAM, 0);
        assert(serverFd_ >= 0);

        int reuse = 1;
        assert(setsockopt(
            serverFd_,
            SOL_SOCKET,
            SO_REUSEADDR,
            &reuse,
            sizeof(reuse)) == 0);

        sockaddr_in address{};
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        address.sin_port = 0;

        assert(bind(
            serverFd_,
            reinterpret_cast<sockaddr*>(&address),
            sizeof(address)) == 0);

        socklen_t length = sizeof(address);
        assert(getsockname(
            serverFd_,
            reinterpret_cast<sockaddr*>(&address),
            &length) == 0);

        port_ = ntohs(address.sin_port);

        assert(listen(serverFd_, 1) == 0);

        worker_ = std::thread([this]() {
            handleOneRequest();
        });
    }

    ~SingleRequestHttpServer()
    {
        if (worker_.joinable())
        {
            worker_.join();
        }

        if (serverFd_ >= 0)
        {
            close(serverFd_);
        }
    }

    int port() const
    {
        return port_;
    }

    const std::string& request() const
    {
        return request_;
    }

private:
    void handleOneRequest()
    {
        const int clientFd =
            accept(serverFd_, nullptr, nullptr);

        assert(clientFd >= 0);

        char buffer[4096];

        while (true)
        {
            const ssize_t received =
                recv(clientFd, buffer, sizeof(buffer), 0);

            if (received <= 0)
            {
                break;
            }

            request_.append(
                buffer,
                static_cast<std::size_t>(received));

            if (request_.find("\r\n\r\n") != std::string::npos &&
                request_.find("\"copy_only\":false") != std::string::npos)
            {
                break;
            }
        }

        const std::string response =
            "HTTP/1.0 200 OK\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: " +
            std::to_string(responseBody_.size()) +
            "\r\n\r\n" +
            responseBody_;

        send(clientFd, response.data(), response.size(), 0);
        close(clientFd);
    }

    std::string responseBody_;
    int serverFd_ = -1;
    int port_ = 0;
    std::thread worker_;
    std::string request_;
};

RestfulApiRecordingActionBackendConfig makeConfig(
    int port)
{
    RestfulApiRecordingActionBackendConfig config;
    config.backendId = "local-vdr";
    config.host = "127.0.0.1";
    config.port = port;
    config.basePath = "";
    config.readOnly = false;
    config.allowExecution = true;
    return config;
}

RecordingActionRequest makeMoveRequest()
{
    RecordingActionRequest request;
    request.backendId = "local-vdr";
    request.recordingId =
        "Tagesschau/2026-06-17.20.00.10-0.rec";
    request.type = RecordingActionType::Move;
    request.dryRun = false;
    request.parameters["recordingPath"] =
        "Tagesschau/2026-06-17.20.00.10-0.rec";
    request.parameters["targetPath"] =
        "Archiv/Tagesschau";
    return request;
}

RecordingActionBackendPolicy makeAllowedPolicy()
{
    RecordingActionBackendPolicyFactory factory;
    return factory.restfulApiMutationPolicy("local-vdr");
}
}

int main()
{
    SingleRequestHttpServer server("Recording moved!");

    BasicHttpClient httpClient(
        "127.0.0.1",
        server.port());

    RecordingActionBackendExecutorAdapterRegistry registry;

    auto executor =
        std::make_shared<RestfulApiRecordingActionExecutor>(
            "local-vdr",
            "restfulapi",
            makeConfig(server.port()),
            httpClient);

    registry.registerAdapter(executor);

    RecordingActionExecutionService service;

    const RecordingActionExecutionResult result =
        service.execute(
            makeMoveRequest(),
            registry,
            makeAllowedPolicy());

    assert(result.success);
    assert(result.type == RecordingActionType::Move);
    assert(result.backendId == "local-vdr");
    assert(result.recordingId == "Tagesschau/2026-06-17.20.00.10-0.rec");
    assert(result.message == "RESTfulAPI recording action request executed");

    const std::string rawRequest =
        server.request();

    assert(rawRequest.find("POST /recordings/move.json HTTP/1.0") != std::string::npos);
    assert(rawRequest.find("Host: 127.0.0.1:") != std::string::npos);
    assert(rawRequest.find("Accept: application/json") != std::string::npos);
    assert(rawRequest.find("Content-Type: application/json") != std::string::npos);
    assert(rawRequest.find("Content-Length: ") != std::string::npos);
    assert(rawRequest.find("\"source\":\"Tagesschau/2026-06-17.20.00.10-0.rec\"") != std::string::npos);
    assert(rawRequest.find("\"target\":\"Archiv~Tagesschau\"") != std::string::npos);
    assert(rawRequest.find("\"copy_only\":false") != std::string::npos);
    assert(rawRequest.find("Archiv/Tagesschau") == std::string::npos);

    return 0;
}
