#include "PublicRecordingPlaybackTargetResolver.h"
#include "PublicRecordingIdentityRepository.h"
#include "VdrRecordingCacheRepository.h"
#include <algorithm>
#include <utility>

namespace {
bool validBackendId(const std::string& id) {
    if (id.empty() || id.size()>128 || id=="*" || id=="." || id=="..")
        return false;
    return std::all_of(id.begin(),id.end(),[](unsigned char c){
        return (c>='A'&&c<='Z') || (c>='a'&&c<='z') ||
            (c>='0'&&c<='9') || c=='.' || c=='_' || c=='-';
    });
}
bool validRecordingId(const std::string& id) {
    return id.size()==36 && id.compare(0,4,"rec_")==0 &&
        std::all_of(id.begin()+4,id.end(),[](unsigned char c){
            return (c>='0'&&c<='9')||(c>='a'&&c<='f');
        });
}
}

PublicRecordingPlaybackTargetResolver::PublicRecordingPlaybackTargetResolver(
    const PublicRecordingIdentityRepository& identities,
    const VdrRecordingCacheRepository& cache)
    : identities_(identities),cache_(cache) {}

PublicRecordingPlaybackTargetResult PublicRecordingPlaybackTargetResolver::resolve(
    const std::string& authorizedBackendId,
    const std::string& publicRecordingId) const {
    auto result = resolveForRead(authorizedBackendId, publicRecordingId);
    if (result.status == PublicRecordingPlaybackTargetStatus::ready &&
        (result.recording.id.empty() || result.recording.title.empty()))
        return {PublicRecordingPlaybackTargetStatus::notFound, {}};
    return result;
}

PublicRecordingPlaybackTargetResult PublicRecordingPlaybackTargetResolver::resolveForRead(
    const std::string& authorizedBackendId,
    const std::string& publicRecordingId) const {
    if (!validBackendId(authorizedBackendId) ||
        !validRecordingId(publicRecordingId))
        return {PublicRecordingPlaybackTargetStatus::invalidRequest,{}};

    const auto native = identities_.findNativeForPublicId(
        authorizedBackendId,publicRecordingId);
    if (!native) return {PublicRecordingPlaybackTargetStatus::notFound,{}};

    // A public ID maps to backendNativeId, never the media-service id.
    // Read-only detail permits an empty media id/title; playback does not.
    // In all cases require a fresh scoped cache entry before readout.
    VdrRecording record;
    if (!cache_.findByBackendNativeId(authorizedBackendId,*native,record) ||
        record.backendId!=authorizedBackendId ||
        record.backendNativeId!=*native)
        return {PublicRecordingPlaybackTargetStatus::notFound,{}};

    // Reject a binding that moved/deleted during the lookup.
    const auto stillBound=identities_.find(authorizedBackendId,*native);
    if (!stillBound || *stillBound!=publicRecordingId)
        return {PublicRecordingPlaybackTargetStatus::notFound,{}};
    return {PublicRecordingPlaybackTargetStatus::ready,std::move(record)};
}
