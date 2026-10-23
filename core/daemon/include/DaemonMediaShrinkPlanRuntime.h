#pragma once

#include "MediaShrink.h"
#include "MediaShrinkOperationSelection.h"
#include "MediaShrinkPlanApiRuntime.h"
#include "MediaShrinkReviewRepository.h"

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
        ResolveRecording resolve, BackendAvailable available,
        vdrsuite::addons::MediaShrinkReviewRepository* reviews = nullptr)
        : roots_(std::move(roots)), resolve_(std::move(resolve)),
          available_(std::move(available)), reviews_(reviews) {}

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

    // Canonical backend-local source revision. HTTP callers never receive a
    // filesystem path or select a root. Used only by Core mutation admission.
    std::optional<std::string> trustedSourceFingerprint(
        const std::string& backend, const std::string& recordingId) const {
        const auto root = roots_.find(backend);
        if (root == roots_.end() || !available_ || !available_(backend) ||
            !resolve_ ||
            !vdrsuite::mediatools::shrinkOpaqueId(recordingId, "rec_"))
            return std::nullopt;
        const auto native = resolve_(backend, recordingId);
        if (!native) return std::nullopt;
        namespace fs = std::filesystem;
        const fs::path videoRoot(root->second), absolute(*native);
        if (!videoRoot.is_absolute() ||
            videoRoot.lexically_normal() != videoRoot ||
            !absolute.is_absolute() ||
            absolute.lexically_normal() != absolute)
            return std::nullopt;
        const fs::path relative = absolute.lexically_relative(videoRoot);
        if (relative.empty() || relative.is_absolute() ||
            relative.extension() != ".rec" ||
            std::any_of(relative.begin(), relative.end(),
                [](const fs::path& part) {
                    return part == "." || part == "..";
                }))
            return std::nullopt;
        try {
            return vdrsuite::mediatools::shrinkSourceFingerprint(
                vdrsuite::mediatools::inspectShrink(videoRoot, relative));
        } catch (...) { return std::nullopt; }
    }

    // INTERNAL trusted Core -> authenticated Agent resolution only. Never
    // attach these native filesystem coordinates to Public API responses or
    // command assignment payloads. Reuses the canonical public-recording
    // identity and live snapshot; a stale fingerprint refuses dispatch.
    struct TrustedRecordingPaths {
        std::filesystem::path videoRoot;
        std::filesystem::path relativeRecording;
    };
    std::optional<TrustedRecordingPaths> trustedSourcePaths(
        const std::string& backend, const std::string& recordingId,
        const std::string& expectedFingerprint) const {
        if (!vdrsuite::mediatools::shrinkOpaqueId(recordingId, "rec_") ||
            expectedFingerprint.size() != 26 ||
            !available_ || !available_(backend) || !resolve_)
            return std::nullopt;
        const auto root = roots_.find(backend);
        if (root == roots_.end()) return std::nullopt;
        const auto recording = resolve_(backend, recordingId);
        if (!recording) return std::nullopt;
        namespace fs = std::filesystem;
        const fs::path videoRoot(root->second), absolute(*recording);
        if (!videoRoot.is_absolute() ||
            videoRoot.lexically_normal() != videoRoot ||
            !absolute.is_absolute() ||
            absolute.lexically_normal() != absolute)
            return std::nullopt;
        const fs::path relative = absolute.lexically_relative(videoRoot);
        if (relative.empty() || relative.is_absolute() ||
            relative.extension() != ".rec" ||
            relative.lexically_normal() != relative ||
            relative.parent_path().empty() ||
            std::any_of(relative.begin(), relative.end(),
                [](const fs::path& part) {
                    return part == "." || part == "..";
                }))
            return std::nullopt;
        try {
            const auto plan = vdrsuite::mediatools::inspectShrink(
                videoRoot, relative);
            if (vdrsuite::mediatools::shrinkSourceFingerprint(plan) !=
                expectedFingerprint) return std::nullopt;
            return TrustedRecordingPaths{videoRoot, relative};
        } catch (...) { return std::nullopt; }
    }

    ApiResponse reviewStatus(const std::string& actor,
                             const std::string& backend,
                             const std::string& reviewId) const {
        if (!reviews_) return MediaShrinkPlanApiRuntime::problem(
            503, "Media Shrink review unavailable.");
        const auto result = reviews_->find(actor, backend, reviewId);
        if (result.status ==
            vdrsuite::addons::MediaShrinkReviewStatus::notFound ||
            result.status ==
            vdrsuite::addons::MediaShrinkReviewStatus::invalid)
            return MediaShrinkPlanApiRuntime::problem(
                404, "Resource not found.");
        if (!result.ok())
            return MediaShrinkPlanApiRuntime::problem(
                503, "Media Shrink review unavailable.");
        const auto& review = result.review;
        if (!review.publishedRecordingId.empty() &&
            !vdrsuite::mediatools::shrinkOpaqueId(
                review.publishedRecordingId, "rec_"))
            return MediaShrinkPlanApiRuntime::problem(
                503, "Media Shrink review unavailable.");
        return MediaShrinkPlanApiRuntime::json(200,
            "{\"backendId\":\"" + backend +
            "\",\"reviewId\":\"" + review.reviewId +
            "\",\"recordingId\":\"" + review.recordingId +
            "\",\"state\":\"" + review.state +
            "\",\"revision\":" + std::to_string(review.revision) +
            ",\"readOnly\":true,\"publishAvailable\":false,"
            "\"publishedRecordingId\":\"" +
            review.publishedRecordingId + "\"}");
    }

private:
    const std::map<std::string, std::string> roots_;
    const ResolveRecording resolve_;
    const BackendAvailable available_;
    vdrsuite::addons::MediaShrinkReviewRepository* reviews_ = nullptr;
};
