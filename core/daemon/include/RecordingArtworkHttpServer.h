#pragma once

#include "IEpgArtworkHttpProvider.h"
#include "IHttpServer.h"
#include "VdrRecordingArtworkService.h"
#include "ArtworkPreviewCache.h"

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
        std::map<std::string, std::string> artworkRootsByBackend,
        std::string previewDirectory = "/var/cache/vdr-suite/artwork-previews");

    HttpServerResponse handleRequest(
        const HttpServerRequest& request) const override;

private:
    std::unique_ptr<IHttpServer> delegate_;
    IEpgArtworkHttpProvider* epgArtworkProvider_;
    VdrRecordingArtworkService artworkService_;
    ArtworkPreviewCache previews_;
};
