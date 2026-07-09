#pragma once

#include "DashboardController.h"

#include <string>

class VdrRecordingCacheRepository;

class VdrRecordingFolderController
{
public:
    explicit VdrRecordingFolderController(
        VdrRecordingCacheRepository& repository);

    ApiResponse getStatus(
        const std::string& backendId);

    ApiResponse getFolder(
        const std::string& backendId,
        const std::string& path,
        int limit,
        int offset);

private:
    VdrRecordingCacheRepository& repository_;
};
