#pragma once

#include "MediaShrink.h"
#include "MediaShrinkPlanApiRuntime.h"

#include <algorithm>
#include <filesystem>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <utility>

// Read-only local preflight using the same canonical recording-ID resolver
// and explicit local video-root policy as Media Check. Execution remains
// unregistered until authorized Core Operations and Agent fencing exist.
class DaemonMediaShrinkPlanRuntime final {
public:
    using ResolveRecording = std::function<std::optional<std::filesystem::path>(
        const std::string&, const std::string&)>;
    using BackendAvailable = std::function<bool(const std::string&)>;

    DaemonMediaShrinkPlanRuntime(
        std::map<std::string, std::string> roots,
        ResolveRecording resolve, BackendAvailable available)
        : roots_(std::move(roots)), resolve_(std::move(resolve)),
          available_(std::move(available)) {}

    ApiResponse capability(const std::string& backend) const {
        const bool ready = roots_.count(backend) && available_ &&
                           available_(backend);
        // "available" must remain false: no stage/publish mutation is enabled.
        return MediaShrinkPlanApiRuntime::json(200,
            "{\"backendId\":\"" + backend +
            "\",\"capability\":\"addons.media.shrink\",\"available\":false,"
            "\"planningAvailable\":" + (ready ? "true" : "false") + "}");
    }

    ApiResponse plan(const std::string& /*actor*/, const std::string& backend,
                     const std::string& recordingId) const {
        const auto root = roots_.find(backend);
        if (root == roots_.end() || !available_ || !available_(backend))
            return MediaShrinkPlanApiRuntime::problem(503,
                "Local Media Shrink planning unavailable.");
        if (!resolve_) return MediaShrinkPlanApiRuntime::problem(503,
            "Recording resolver unavailable.");
        const auto recording = resolve_(backend, recordingId);
        if (!recording)
            return MediaShrinkPlanApiRuntime::problem(404,
                "Recording not in current backend snapshot.");
        namespace fs = std::filesystem;
        const fs::path videoRoot(root->second), absolute(*recording);
        if (!videoRoot.is_absolute() ||
            videoRoot.lexically_normal() != videoRoot ||
            !absolute.is_absolute() ||
            absolute.lexically_normal() != absolute)
            return MediaShrinkPlanApiRuntime::problem(503,
                "Configured recording root invalid.");
        const fs::path relative = absolute.lexically_relative(videoRoot);
        if (relative.empty() || relative.is_absolute() ||
            relative.extension() != ".rec" ||
            std::any_of(relative.begin(), relative.end(),
                [](const fs::path& part) {
                    return part == "." || part == "..";
                }))
            return MediaShrinkPlanApiRuntime::problem(503,
                "Recording outside configured video root.");
        try {
            const auto inspected = vdrsuite::mediatools::inspectShrink(
                videoRoot, relative);
            return MediaShrinkPlanApiRuntime::json(200,
                "{\"backendId\":\"" + backend +
                "\",\"recordingId\":\"" + recordingId +
                "\",\"readOnly\":true,\"stageAvailable\":false,"
                "\"originalPreserved\":true,\"sourceBytes\":" +
                std::to_string(inspected.totalBytes) +
                ",\"segments\":" +
                std::to_string(inspected.segments.size()) + "}");
        } catch (...) {
            // No filesystem details, native paths or candidate tokens leak.
            return MediaShrinkPlanApiRuntime::problem(422,
                "Recording cannot be safely inspected for HEVC.");
        }
    }

private:
    const std::map<std::string, std::string> roots_;
    const ResolveRecording resolve_;
    const BackendAvailable available_;
};
