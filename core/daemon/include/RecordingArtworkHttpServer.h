#pragma once

#include "IHttpServer.h"
#include "VdrRecordingArtworkService.h"

#include <memory>
#include <string>
#include <vector>

class VdrRecordingCacheRepository;

class RecordingArtworkHttpServer : public IHttpServer
{
public:
    RecordingArtworkHttpServer(
        std::unique_ptr<IHttpServer> delegate,
        VdrRecordingCacheRepository& repository,
        std::vector<std::string> artworkRoots);

    HttpServerResponse handleRequest(
        const HttpServerRequest& request) const override;

private:
    std::unique_ptr<IHttpServer> delegate_;
    VdrRecordingArtworkService artworkService_;
};
