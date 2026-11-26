#pragma once

#include "DashboardController.h"
#include "ManualRecordingMetadataApiRuntime.h"
#include "ManualRecordingMetadataAssignmentRepository.h"
#include "VdrRecordingNativeMetadataRepository.h"

#include <functional>
#include <map>
#include <string>
#include <utility>
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

    using ManualMetadataBatchLookup = std::function<
        std::map<std::string, ManualRecordingMetadataAssignment>(
            const std::string& backendId)>;

    explicit VdrRecordingFolderController(
        VdrRecordingCacheRepository& repository,
        NativeMetadataLookup nativeMetadataLookup = {},
        std::vector<std::string> metadataImageAllowedRoots = {})
        : VdrRecordingFolderController(
              repository,
              std::move(nativeMetadataLookup),
              [](const std::string& backendId,
                 const std::string& backendNativeId)
              {
                  return ManualRecordingMetadataApiRuntime::instance()
                      .findSelected(backendId, backendNativeId);
              },
              [](const std::string& backendId)
              {
                  return ManualRecordingMetadataApiRuntime::instance()
                      .findSelectedForBackend(backendId);
              },
              std::move(metadataImageAllowedRoots))
    {
    }

    VdrRecordingFolderController(
        VdrRecordingCacheRepository& repository,
        NativeMetadataLookup nativeMetadataLookup,
        ManualMetadataLookup manualMetadataLookup,
        std::vector<std::string> metadataImageAllowedRoots)
        : VdrRecordingFolderController(
              repository,
              std::move(nativeMetadataLookup),
              std::move(manualMetadataLookup),
              {},
              std::move(metadataImageAllowedRoots))
    {
    }

    VdrRecordingFolderController(
        VdrRecordingCacheRepository& repository,
        NativeMetadataLookup nativeMetadataLookup,
        ManualMetadataLookup manualMetadataLookup,
        ManualMetadataBatchLookup manualMetadataBatchLookup,
        std::vector<std::string> metadataImageAllowedRoots);

    ApiResponse getStatus(
        const std::string& backendId);

    ApiResponse getFolder(
        const std::string& backendId,
        const std::string& path,
        int limit,
        int offset,
        bool homeCards = false);

    // Existing private Web browse route, for opening an opaque Public-v1
    // list item in the established Browser detail/action owner.
    using PublicRecordingNativeLookup = std::function<std::string(
        const std::string& backendId, const std::string& publicRecordingId)>;
    void setPublicRecordingNativeLookup(PublicRecordingNativeLookup lookup)
    {
        publicRecordingNativeLookup_ = std::move(lookup);
    }
    ApiResponse getRecordingForBrowser(
        const std::string& backendId,
        const std::string& publicRecordingId) const;

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
    PublicRecordingNativeLookup publicRecordingNativeLookup_;
    NativeMetadataLookup nativeMetadataLookup_;
    ManualMetadataLookup manualMetadataLookup_;
    ManualMetadataBatchLookup manualMetadataBatchLookup_;
    std::vector<std::string> metadataImageAllowedRoots_;
};
