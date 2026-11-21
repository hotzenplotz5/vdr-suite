#pragma once

#include "DashboardController.h"

#include <algorithm>
#include <functional>
#include <mutex>
#include <regex>
#include <string>
#include <vector>

// Narrow Public API v1 bridge for the read-only Media Check operation.
// Execution and canonical recording resolution are provided by Suite Core.
class MediaCheckApiRuntime final
{
public:
    using Start = std::function<ApiResponse(
        const std::string&, const std::string&,
        const std::string&, const std::string&)>;
    using Status = std::function<ApiResponse(
        const std::string&, const std::string&, const std::string&)>;
    using Capability = std::function<ApiResponse(const std::string&)>;

    static MediaCheckApiRuntime& instance()
    {
        static MediaCheckApiRuntime runtime;
        return runtime;
    }

    void registerHandlers(Start start, Status status, Capability capability)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        start_ = std::move(start);
        status_ = std::move(status);
        capability_ = std::move(capability);
    }

    bool tryHandleGet(
        const std::string& target, const std::string& actorId,
        const std::string& scopedBackend,
        const std::vector<std::string>& scopedBackends,
        ApiResponse& response) const
    {
        std::string backendId, remainder;
        if (!parse(target, backendId, remainder) ||
            (remainder != "/capabilities/media-check" &&
             !validStatusPath(remainder)))
            return false;
        if (target.find('?') != std::string::npos) {
            response = problem(400, "Media Check accepts no query parameters.");
            return true;
        }
        if (actorId.empty()) {
            response = problem(401, "Authentication required.");
            return true;
        }
        if (!scoped(backendId, scopedBackend, scopedBackends)) {
            response = problem(404, "Media Check resource not found.");
            return true;
        }
        if (remainder == "/capabilities/media-check") {
            Capability capability;
            {
                std::lock_guard<std::mutex> lock(mutex_);
                capability = capability_;
            }
            response = capability ? capability(backendId)
                                  : problem(503, "Media Check backend unavailable.");
            return true;
        }
        Status status;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            status = status_;
        }
        response = status
            ? status(actorId, backendId, remainder.substr(
                std::string("/media-checks/").size()))
            : problem(503, "Media Check backend unavailable.");
        return true;
    }

    bool tryHandlePost(
        const std::string& target, const std::string& body,
        const std::string& actorId, const std::string& authorizedBackend,
        const std::string& contentType, ApiResponse& response) const
    {
        std::string backendId, remainder;
        if (!parse(target, backendId, remainder) ||
            remainder != "/media-checks")
            return false;
        if (target.find('?') != std::string::npos ||
            body.size() > 256 || !validJsonContentType(contentType)) {
            response = problem(400, "Invalid Media Check request.");
            return true;
        }
        if (actorId.empty()) {
            response = problem(401, "Authentication required.");
            return true;
        }
        if (authorizedBackend != backendId &&
            authorizedBackend != "*") {
            response = problem(403, "Media Check backend access denied.");
            return true;
        }
        std::string recordingId, mode;
        if (!parseBody(body, recordingId, mode)) {
            response = problem(422,
                "Expected recordingId (opaque rec_ ID) and mode (structure or decode).");
            return true;
        }
        Start start;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            start = start_;
        }
        response = start
            ? start(actorId, backendId, recordingId, mode)
            : problem(503, "Media Check backend unavailable.");
        return true;
    }

    static ApiResponse json(int code, const std::string& body)
    {
        ApiResponse response;
        response.statusCode = code;
        response.contentType = "application/json";
        response.headers["Cache-Control"] = "no-store";
        response.body = body;
        return response;
    }

    static ApiResponse problem(int code, const std::string& message)
    {
        // The messages in this class are fixed strings, not user input.
        return json(code, "{\"error\":\"" + message + "\"}");
    }

private:
    mutable std::mutex mutex_;
    Start start_;
    Status status_;
    Capability capability_;

    static bool parse(const std::string& target,
                      std::string& backendId, std::string& remainder)
    {
        constexpr const char* prefix = "/api/v1/backends/";
        const std::string path = target.substr(0, target.find('?'));
        if (path.compare(0, std::char_traits<char>::length(prefix), prefix) != 0)
            return false;
        const auto start = std::char_traits<char>::length(prefix);
        const auto slash = path.find('/', start);
        if (slash == std::string::npos) return false;
        backendId = path.substr(start, slash - start);
        remainder = path.substr(slash);
        if (backendId.empty() || backendId.size() > 128 ||
            backendId == "." || backendId == ".." || backendId == "*")
            return false;
        return std::all_of(backendId.begin(), backendId.end(),
            [](unsigned char c) {
                return (c >= '0' && c <= '9') ||
                    (c >= 'A' && c <= 'Z') ||
                    (c >= 'a' && c <= 'z') ||
                    c == '_' || c == '-' || c == '.';
            });
    }

    static bool validStatusPath(const std::string& remainder)
    {
        constexpr const char* prefix = "/media-checks/chk_";
        const std::string prefixText(prefix);
        if (remainder.compare(0, prefixText.size(), prefixText) != 0 ||
            remainder.size() != prefixText.size() + 32) return false;
        return std::all_of(remainder.begin() + prefixText.size(),
            remainder.end(), [](char c) {
                return (c >= '0' && c <= '9') ||
                    (c >= 'a' && c <= 'f');
            });
    }

    static bool scoped(const std::string& backend,
                       const std::string& one,
                       const std::vector<std::string>& many)
    {
        return one == backend || one == "*" ||
            std::find(many.begin(), many.end(), backend) != many.end() ||
            std::find(many.begin(), many.end(), "*") != many.end();
    }

    static bool validJsonContentType(const std::string& text)
    {
        return text == "application/json" ||
            text.compare(0, 17, "application/json;") == 0;
    }

    static bool parseBody(
        const std::string& body, std::string& recordingId, std::string& mode)
    {
        // Strict bounded two-field JSON: never accept browser filesystem paths.
        static const std::regex first(
            R"MC(^\s*\{\s*"recordingId"\s*:\s*"(rec_[0-9a-f]{32})"\s*,\s*"mode"\s*:\s*"(structure|decode)"\s*\}\s*$)MC");
        static const std::regex second(
            R"MC(^\s*\{\s*"mode"\s*:\s*"(structure|decode)"\s*,\s*"recordingId"\s*:\s*"(rec_[0-9a-f]{32})"\s*\}\s*$)MC");
        std::smatch match;
        if (std::regex_match(body, match, first)) {
            recordingId = match[1];
            mode = match[2];
            return true;
        }
        if (std::regex_match(body, match, second)) {
            recordingId = match[2];
            mode = match[1];
            return true;
        }
        return false;
    }
};
