#pragma once

#include "DashboardController.h"
#include <algorithm>
#include <functional>
#include <mutex>
#include <regex>
#include <string>
#include <utility>
#include <vector>

// Only a read-only, backend-local planning endpoint. No stage/publish
// execution or arbitrary path supplied by a caller is ever accepted.
class MediaShrinkPlanApiRuntime final {
public:
    using Plan = std::function<ApiResponse(const std::string&,
                                            const std::string&,
                                            const std::string&)>;
    using Capability = std::function<ApiResponse(const std::string&)>;
    using ReviewStatus = std::function<ApiResponse(const std::string&,
                                                   const std::string&,
                                                   const std::string&)>;

    static MediaShrinkPlanApiRuntime& instance() {
        static MediaShrinkPlanApiRuntime runtime;
        return runtime;
    }
    void registerHandlers(Plan plan, Capability capability,
                          ReviewStatus reviewStatus = {}) {
        std::lock_guard<std::mutex> lock(mutex_);
        plan_ = std::move(plan);
        capability_ = std::move(capability);
        reviewStatus_ = std::move(reviewStatus);
    }

    bool tryHandleGet(const std::string& target,
                      const std::string& actor,
                      const std::string& authorizedBackend,
                      const std::vector<std::string>& authorizedBackends,
                      ApiResponse& response) const {
        std::string backend, suffix;
        if (!parse(target, backend, suffix)) return false;
        const bool capabilityRequest = suffix == "/capabilities/media-shrink";
        const std::string reviewPrefix = "/media-shrink-reviews/";
        const bool reviewRequest =
            suffix.compare(0, reviewPrefix.size(), reviewPrefix) == 0;
        if (!capabilityRequest && !reviewRequest) return false;
        if (target.find('?') != std::string::npos) {
            response = problem(400, "Query parameters are not accepted.");
            return true;
        }
        if (actor.empty()) {
            response = problem(401, "Authentication required.");
            return true;
        }
        if (!scoped(backend, authorizedBackend, authorizedBackends)) {
            response = problem(404, "Resource not found.");
            return true;
        }
        if (reviewRequest) {
            const std::string reviewId = suffix.substr(reviewPrefix.size());
            if (reviewId.size() != 36 ||
                reviewId.compare(0, 4, "shk_") != 0 ||
                !std::all_of(reviewId.begin() + 4, reviewId.end(),
                    [](unsigned char ch) {
                        return (ch >= '0' && ch <= '9') ||
                               (ch >= 'a' && ch <= 'f');
                    })) {
                response = problem(404, "Resource not found.");
                return true;
            }
            ReviewStatus read;
            {
                std::lock_guard<std::mutex> lock(mutex_);
                read = reviewStatus_;
            }
            response = read ? read(actor, backend, reviewId)
                            : problem(503, "Media Shrink review unavailable.");
            return true;
        }
        Capability capability;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            capability = capability_;
        }
        response = capability ? capability(backend)
                              : problem(503, "Media Shrink unavailable.");
        return true;
    }

    bool tryHandlePost(const std::string& target, const std::string& body,
                       const std::string& actor,
                       const std::string& authorizedBackend,
                       const std::string& contentType,
                       ApiResponse& response) const {
        std::string backend, suffix;
        if (!parse(target, backend, suffix) ||
            suffix != "/media-shrink-plans") return false;
        if (target.find('?') != std::string::npos ||
            body.size() > 128 ||
            (contentType != "application/json" &&
             contentType.compare(0, 17, "application/json;") != 0)) {
            response = problem(400, "Invalid Media Shrink plan request.");
            return true;
        }
        if (actor.empty()) {
            response = problem(401, "Authentication required.");
            return true;
        }
        if (authorizedBackend != backend && authorizedBackend != "*") {
            response = problem(403, "Backend access denied.");
            return true;
        }
        static const std::regex shape(
            R"MS(^\s*\{\s*"recordingId"\s*:\s*"(rec_[0-9a-f]{32})"\s*\}\s*$)MS");
        std::smatch match;
        if (!std::regex_match(body, match, shape)) {
            response = problem(422, "Expected only an opaque recordingId.");
            return true;
        }
        Plan plan;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            plan = plan_;
        }
        response = plan ? plan(actor, backend, match[1].str())
                        : problem(503, "Media Shrink planning unavailable.");
        return true;
    }

    static ApiResponse json(int status, const std::string& body) {
        ApiResponse response;
        response.statusCode = status;
        response.contentType = "application/json";
        response.headers["Cache-Control"] = "no-store";
        response.body = body;
        return response;
    }
    static ApiResponse problem(int status, const std::string& text) {
        // Fixed internal strings only, never caller-provided data.
        return json(status, "{\"error\":\"" + text + "\"}");
    }

private:
    mutable std::mutex mutex_;
    Plan plan_;
    Capability capability_;
    ReviewStatus reviewStatus_;

    static bool scoped(const std::string& backend, const std::string& one,
                       const std::vector<std::string>& many) {
        return backend == one || one == "*" ||
            std::find(many.begin(), many.end(), backend) != many.end() ||
            std::find(many.begin(), many.end(), "*") != many.end();
    }
    static bool parse(const std::string& target, std::string& backend,
                      std::string& suffix) {
        constexpr const char* prefix = "/api/v1/backends/";
        const std::string path = target.substr(0, target.find('?'));
        if (path.compare(0, 17, prefix) != 0) return false;
        const auto slash = path.find('/', 17);
        if (slash == std::string::npos) return false;
        backend = path.substr(17, slash - 17);
        suffix = path.substr(slash);
        return !backend.empty() && backend.size() <= 128 &&
            backend != "." && backend != ".." && backend != "*" &&
            std::all_of(backend.begin(), backend.end(), [](unsigned char c) {
                return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') ||
                       (c >= 'a' && c <= 'z') || c == '-' ||
                       c == '_' || c == '.';
            });
    }
};
