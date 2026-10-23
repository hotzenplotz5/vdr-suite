#pragma once

#include "VdrRecording.h"

#include <optional>
#include <string>
#include <vector>

// Read-only, conservative evidence for a recording that changed its backend
// native path after a provider-confirmed RENAME or MOVE. This is not an
// identity authority. Only PublicRecordingIdentityRepository may rebind IDs.
struct PublicRecordingActionReadback final
{
    static std::optional<std::string> relocatedNativeId(
        const VdrRecording& before,
        const std::vector<VdrRecording>& providerReadback)
    {
        // An incomplete/growing source can change extent independently of
        // a rename. Refuse to guess which target belongs to a public ID.
        if (before.backendId.empty() || before.backendNativeId.empty() ||
            before.startTime.empty() || before.startTime == "-1" ||
            !before.recordingDurationKnown || before.durationSeconds <= 0 ||
            before.sizeMb <= 0)
            return std::nullopt;

        const auto leaf = [](const std::string& path) {
            const auto pos = path.find_last_of('/');
            return pos == std::string::npos ? path : path.substr(pos + 1U);
        };
        const std::string originalLeaf = leaf(before.backendNativeId);
        if (originalLeaf.empty() || originalLeaf.size() < 5U ||
            originalLeaf.compare(originalLeaf.size() - 4U, 4U, ".rec") != 0)
            return std::nullopt;

        std::optional<std::string> unique;
        for (const VdrRecording& current : providerReadback)
        {
            if (current.backendId != before.backendId)
                continue;

            // The old recording is still present: an accepted provider HTTP
            // response alone is not a verified native rename/move.
            if (current.backendNativeId == before.backendNativeId)
                return std::nullopt;

            if (current.backendNativeId.empty() ||
                leaf(current.backendNativeId) != originalLeaf ||
                current.startTime != before.startTime ||
                !current.recordingDurationKnown ||
                current.durationSeconds != before.durationSeconds ||
                current.sizeMb != before.sizeMb ||
                current.title.empty())
                continue;

            // Two plausible matching paths are ambiguous. Never rebind.
            if (unique)
                return std::nullopt;
            unique = current.backendNativeId;
        }
        return unique;
    }

    static bool deletedNativeId(
        const VdrRecording& before,
        const std::vector<VdrRecording>& providerReadback)
    {
        if (before.backendId.empty() || before.backendNativeId.empty())
            return false;
        for (const VdrRecording& current : providerReadback)
        {
            if (current.backendId == before.backendId &&
                current.backendNativeId == before.backendNativeId)
                return false;
        }
        return true;
    }
};
