#pragma once

#include "DashboardController.h"
#include "ManualRecordingMetadataAssignmentRepository.h"
#include "VdrRecordingNativeMetadataRepository.h"

#include <functional>
#include <string>
#include <vector>

class VdrRecordingCacheRepository;

class VdrRecordingFolderController
{
public:
    using NativeMetadataLookup = std::function<
        VdrRecordingNativeMetadataRecord(
            const std::string& backendId,
            const std::string& backendNativeId)>;

    using ManualMetadataLookup = std::function<
        ManualRecordingMetadataAssignment(
            const std::string& backendId,
            const std::string& backendNativeId)>;

    explicit VdrRecordingFolderController(
        VdrRecordingCacheRepository& repository,
        NativeMetadataLookup nativeMetadataLookup = {},
        ManualMetadataLookup manualMetadataLookup = {},
        std::vector<std::string> metadataImageAllowedRoots = {});

    ApiResponse getStatus(
        const std::string& backendId);

    ApiResponse getFolder(
        const std::string& backendId,
        const std::string& path,
        int limit,
        int offset);

    ApiResponse getMetadata(
        const std::string& backendId,
        const std::string& backendNativeId) const;

    ApiResponse getMetadataImage(
        const std::string& backendId,
        const std::string& backendNativeId,
        const std::string& kind,
        int index) const;

private:
    VdrRecordingCacheRepository& repository_;
    NativeMetadataLookup nativeMetadataLookup_;
    ManualMetadataLookup manualMetadataLookup_;
    std::vector<std::string> metadataImageAllowedRoots_;
};
