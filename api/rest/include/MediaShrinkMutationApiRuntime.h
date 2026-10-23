#pragma once

#include "DashboardController.h"
#include "MediaShrinkAgentCommand.h"
#include <functional>
#include <mutex>
#include <regex>
#include <string>
#include <utility>

// Public API parsing only. It does not resolve recording paths, grant rights,
// create operations or dispatch FFmpeg. An unavailable Core mutation authority
// is a hard 503 and cannot be bypassed by a module manifest or browser input.
class MediaShrinkMutationApiRuntime final {
public:
    struct StageRequest {
        std::string recordingId;
        std::string encoder;
        unsigned maxHeight = 0;
        bool nightMode = false;
    };
    using Stage = std::function<ApiResponse(
        const std::string& actor, const std::string& backend,
        const StageRequest& request, const std::string& idempotencyKey)>;
    using Publish = std::function<ApiResponse(
        const std::string& actor, const std::string& backend,
        const std::string& reviewId, std::uint64_t expectedRevision,
        const std::string& idempotencyKey)>;

    static MediaShrinkMutationApiRuntime& instance() {
        static MediaShrinkMutationApiRuntime runtime;
        return runtime;
    }

    void registerHandlers(Stage stage, Publish publish) {
        std::lock_guard<std::mutex> lock(mutex_);
        stage_ = std::move(stage);
        publish_ = std::move(publish);
    }

    bool tryHandlePost(
        const std::string& target, const std::string& body,
        const std::string& actor, const std::string& authorizedBackend,
        const std::string& idempotencyKey, const std::string& ifMatch,
        const std::string& contentType, ApiResponse& response) const
    {
        const std::string path = target.substr(0, target.find('?'));
        constexpr const char* prefix = "/api/v1/backends/";
        if (path.compare(0, 17, prefix) != 0) return false;
        const auto slash = path.find('/', 17);
        if (slash == std::string::npos) return false;
        const std::string backend = path.substr(17, slash - 17);
        const std::string suffix = path.substr(slash);
        const bool staging = suffix == "/media-shrink-reviews";
        const std::string reviewPrefix = "/media-shrink-reviews/";
        const bool publishing =
            suffix.compare(0, reviewPrefix.size(), reviewPrefix) == 0 &&
            suffix.size() > reviewPrefix.size() &&
            suffix.compare(suffix.size() - 8, 8, "/publish") == 0;
        if (!staging && !publishing) return false;
        if (target.find('?') != std::string::npos || body.size() > 256 ||
            (contentType != "application/json" &&
             contentType.compare(0, 17, "application/json;") != 0)) {
            response = problem(400, "Invalid HEVC mutation request.");
            return true;
        }
        if (actor.empty()) {
            response = problem(401, "Authentication required.");
            return true;
        }
        if (!safeBackend(backend) ||
            (authorizedBackend != backend && authorizedBackend != "*")) {
            response = problem(403, "Backend access denied.");
            return true;
        }
        if (!safeKey(idempotencyKey)) {
            response = problem(400, "Valid Idempotency-Key required.");
            return true;
        }
        if (staging) {
            if (!ifMatch.empty()) {
                response = problem(400, "If-Match not accepted for staging.");
                return true;
            }
            static const std::regex shape(
                R"MS(^\s*\{\s*"recordingId"\s*:\s*"(rec_[0-9a-f]{32})"\s*,\s*"encoder"\s*:\s*"(software|vaapi|qsv|nvenc)"\s*,\s*"maxHeight"\s*:\s*(0|[1-9][0-9]{0,3})\s*,\s*"nightMode"\s*:\s*(true|false)\s*\}\s*$)MS");
            std::smatch match;
            if (!std::regex_match(body, match, shape)) {
                response = problem(422, "Invalid HEVC staging selection.");
                return true;
            }
            StageRequest request;
            request.recordingId = match[1].str();
            request.encoder = match[2].str();
            request.maxHeight =
                static_cast<unsigned>(std::stoul(match[3].str()));
            request.nightMode = match[4].str() == "true";
            const vdrsuite::mediatools::OpaqueShrinkSelection selection{
                request.recordingId, "",
                vdrsuite::mediatools::ShrinkRequestAction::stage,
                request.encoder, request.maxHeight, request.nightMode
            };
            if (vdrsuite::mediatools::inspectOpaqueShrinkSelection(selection) !=
                vdrsuite::mediatools::ShrinkSelectionStatus::candidateOnly) {
                response = problem(422, "Invalid HEVC staging selection.");
                return true;
            }
            Stage stage;
            {
                std::lock_guard<std::mutex> lock(mutex_);
                stage = stage_;
            }
            response = stage ? stage(actor, backend, request, idempotencyKey)
                             : problem(503, "HEVC staging not authorized.");
            return true;
        }
        const std::string reviewId = suffix.substr(
            reviewPrefix.size(),
            suffix.size() - reviewPrefix.size() - 8);
        if (!vdrsuite::mediatools::shrinkOpaqueId(reviewId, "shk_")) {
            response = problem(404, "HEVC review not found.");
            return true;
        }
        static const std::regex revision(
            R"MS(^\s*\{\s*"revision"\s*:\s*([1-9][0-9]{0,17})\s*\}\s*$)MS");
        std::smatch match;
        if (!std::regex_match(body, match, revision)) {
            response = problem(422, "Invalid HEVC review revision.");
            return true;
        }
        const auto revisionNumber = std::stoull(match[1].str());
        if (revisionNumber != 1 ||
            ifMatch != "\"" + std::to_string(revisionNumber) + "\"") {
            response = problem(409, "HEVC review revision mismatch.");
            return true;
        }
        Publish publish;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            publish = publish_;
        }
        response = publish
            ? publish(actor, backend, reviewId, revisionNumber, idempotencyKey)
            : problem(503, "HEVC publication not authorized.");
        return true;
    }

private:
    static ApiResponse problem(int status, const std::string& message) {
        ApiResponse response;
        response.statusCode = status;
        response.contentType = "application/json";
        response.headers["Cache-Control"] = "no-store";
        response.body = "{\"error\":\"" + message + "\"}";
        return response;
    }
    static bool safeKey(const std::string& key) {
        if (key.size() < 8 || key.size() > 128) return false;
        for (unsigned char c : key)
            if (!((c >= 'A' && c <= 'Z') ||
                  (c >= 'a' && c <= 'z') ||
                  (c >= '0' && c <= '9') ||
                  c == '_' || c == '-' || c == '.' || c == ':'))
                return false;
        return true;
    }
    static bool safeBackend(const std::string& key) {
        if (key.empty() || key.size() > 128 || key == "." ||
            key == ".." || key == "*") return false;
        for (unsigned char c : key)
            if (!((c >= 'A' && c <= 'Z') ||
                  (c >= 'a' && c <= 'z') ||
                  (c >= '0' && c <= '9') ||
                  c == '_' || c == '-' || c == '.'))
                return false;
        return true;
    }
    mutable std::mutex mutex_;
    Stage stage_;
    Publish publish_;
};
