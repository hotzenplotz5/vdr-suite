#pragma once

#include "IHttpServer.h"
#include "VdrRecordingArtworkService.h"

#include <map>
#include <memory>
#include <string>

class VdrRecordingCacheRepository;

class RecordingArtworkHttpServer : public IHttpServer
{
public:
    RecordingArtworkHttpServer(
        std::unique_ptr<IHttpServer> delegate,
        VdrRecordingCacheRepository& repository,
        std::map<std::string, std::string> artworkRootsByBackend);

    HttpServerResponse handleRequest(
        const HttpServerRequest& request) const override;

private:
    std::unique_ptr<IHttpServer> delegate_;
    VdrRecordingArtworkService artworkService_;
};
