#pragma once

#include "DashboardController.h"

#include <string>
#include <vector>

class EpgArtworkRepository;

class EpgArtworkController
{
public:
    explicit EpgArtworkController(EpgArtworkRepository& repository);
    EpgArtworkController(
        EpgArtworkRepository& repository,
        std::vector<std::string> allowedRoots);

    ApiResponse getArtwork(
        const std::string& backendId,
        const std::string& channelId,
        const std::string& eventId) const;

private:
    EpgArtworkRepository& repository_;
    std::vector<std::string> allowedRoots_;
};
