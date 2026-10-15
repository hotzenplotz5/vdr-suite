#pragma once

#include "VdrRecordingCacheRepository.h"

#include <optional>
#include <string>
#include <vector>

// The Web search index and the Home/Genre Recording inventory share the
// persisted, backend-scoped VDR Recording cache. The fast startup snapshot
// deliberately omits Recordings and is not a Recording liveness authority.
// Refuse a refreshing/failed cache rather than returning a misleading
// successful zero-result search.
namespace PublicSearchRecordingInventory
{
inline std::optional<std::vector<VdrRecording>> readReady(
    const VdrRecordingCacheRepository& cache,
    const std::string& backendId)
{
    if (backendId.empty() || backendId == "*")
        return std::nullopt;

    const auto status = cache.statusForBackend(backendId);
    if (status.backendId != backendId || status.state != "ready")
        return std::nullopt;

    auto recordings = cache.findAllForBackend(backendId);
    if (status.totalCount > 0 && recordings.empty())
        return std::nullopt;
    return recordings;
}
} // namespace PublicSearchRecordingInventory
