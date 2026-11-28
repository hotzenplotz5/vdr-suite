#pragma once

#include "DashboardController.h"
#include "ManualRecordingMetadataApiRuntime.h"
#include "ManualRecordingMetadataAssignmentRepository.h"
#include "VdrRecordingNativeMetadataRepository.h"

#include <functional>
#include <map>
#include <optional>
#include <string>
#include <utility>
#include <vector>

class VdrRecordingCacheRepository;
struct VdrRecording;

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
    // Existing Web folder DTOs expose the same durable public ID as v1.
    using PublicRecordingIdLookup = std::function<std::string(
        const std::string& backendId, const std::string& backendNativeId)>;
    void setPublicRecordingIdLookup(PublicRecordingIdLookup lookup)
    {
        publicRecordingIdLookup_ = std::move(lookup);
    }
    ApiResponse getRecordingForBrowser(
        const std::string& backendId,
        const std::string& publicRecordingId) const;

    // Shared persisted native-detail lookup; no provider fetch.
    VdrRecordingNativeMetadataRecord findNativeMetadata(
        const std::string& backendId, const std::string& backendNativeId) const
    {
        return nativeMetadataLookup_
            ? nativeMetadataLookup_(backendId, backendNativeId)
            : VdrRecordingNativeMetadataRecord{};
    }

    struct PublicMetadataArtwork
    {
        std::string artworkId;
        std::string kind;
        int index = 0;
        std::string orientation;
        int width = 0;
        int height = 0;
    };

    // The detail projection and binary GET use exactly the same current
    // metadata evidence. These methods receive an already resolved recording.
    static std::vector<PublicMetadataArtwork> publicMetadataArtwork(
        const VdrRecording& recording,
        const ManualRecordingMetadataAssignment& manual,
        const VdrRecordingNativeMetadataRecord& native);
    // Search uses the same selected manual-portrait identity as detail and
    // the binary reader, with the persisted cast ordinal and revision.
    static std::string publicSearchPreferredArtworkId(
        const VdrRecording& recording, const std::string& localPath,
        int width, int height, bool manualSelected, int manualRevision);
    // Same gallery/preferred identity as the detail and binary reader.
    static std::string publicNativeCardArtworkId(
        const VdrRecording& recording, const std::string& path,
        int width, int height, int galleryIndex);
    static std::string publicManualPortraitArtworkId(
        const VdrRecording& recording, int index, int revision,
        const std::string& localPath);
    std::optional<ApiResponse> loadPublicMetadataArtwork(
        const VdrRecording& recording,
        const ManualRecordingMetadataAssignment& manual,
        const VdrRecordingNativeMetadataRecord& native,
        const std::string& artworkId) const;

    // Provider identity remains internal. Both the detail's trailer hint
    // and the public trailer lookup use this same current metadata evidence.
    static std::pair<std::string, std::string> metadataTrailerIdentity(
        const VdrRecording& recording,
        const ManualRecordingMetadataAssignment& manual,
        const VdrRecordingNativeMetadataRecord& native);

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
    PublicRecordingIdLookup publicRecordingIdLookup_;
    NativeMetadataLookup nativeMetadataLookup_;
    ManualMetadataLookup manualMetadataLookup_;
    ManualMetadataBatchLookup manualMetadataBatchLookup_;
    std::vector<std::string> metadataImageAllowedRoots_;
};
