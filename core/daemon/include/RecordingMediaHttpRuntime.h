#pragma once

#include <functional>
#include <memory>
#include <string>

class ApiRouter;
class Database;
class IHttpServer;
class SimpleHttpListener;
class VdrRecordingQueryService;

int runRecordingMediaHttpRuntime(
    Database& database,
    ApiRouter& apiRouter,
    VdrRecordingQueryService& recordingQueryService,
    std::unique_ptr<IHttpServer>& httpServer,
    std::unique_ptr<SimpleHttpListener>& httpListener,
    const std::string& listenHost,
    int listenPort,
    std::function<bool()> shouldStop,
    std::function<void()> onTick);
