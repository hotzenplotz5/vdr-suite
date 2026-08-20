#include "MediaTranscodeSettingsApiRuntime.h"

#include "MediaTranscodeBackendSettingsService.h"
#include "Database.h"

#include <cctype>
#include <map>
#include <set>
#include <sstream>
#include <string>

namespace
{
constexpr const char* RouteSuffix = "/settings/media-transcode";
constexpr const char* RoutePrefix = "/api/backends/";

std::string requestPath(const std::string& target)
{
    const std::size_t query = target.find('?');
    return query == std::string::npos ? target : target.substr(0, query);
}

bool routeBackendId(
    const std::string& requestTarget,
    std::string& backendId)
{
    const std::string path = requestPath(requestTarget);
    const std::string prefix(RoutePrefix);
    const std::string suffix(RouteSuffix);
    if (path.size() <= prefix.size() + suffix.size() ||
        path.compare(0, prefix.size(), prefix) != 0 ||
        path.compare(path.size() - suffix.size(), suffix.size(), suffix) != 0)
    {
        return false;
    }

    backendId = path.substr(
        prefix.size(),
        path.size() - prefix.size() - suffix.size());
    return MediaTranscodeBackendSettingsService::validBackendId(backendId);
}

std::string jsonEscape(const std::string& value)
{
    std::string escaped;
    escaped.reserve(value.size());
    for (const char character : value) {
        switch (character) {
        case '"': escaped += "\\\""; break;
        case '\\': escaped += "\\\\"; break;
        case '\b': escaped += "\\b"; break;
        case '\f': escaped += "\\f"; break;
        case '\n': escaped += "\\n"; break;
        case '\r': escaped += "\\r"; break;
        case '\t': escaped += "\\t"; break;
        default:
            if (static_cast<unsigned char>(character) >= 0x20U)
                escaped.push_back(character);
            break;
        }
    }
    return escaped;
}

const char* jsonBool(bool value)
{
    return value ? "true" : "false";
}

std::string serialize(const MediaTranscodeBackendSettingsSnapshot& settings)
{
    const MediaTranscodePolicyDiagnostics& d = settings.diagnostics;
    std::ostringstream output;
    output << "{\"backendId\":\"" << jsonEscape(settings.backendId)
           << "\",\"managed\":" << jsonBool(settings.managed)
           << ",\"managedMode\":";
    if (settings.managed) {
        output << "\"" << jsonEscape(settings.managedMode) << "\"";
    }
    else {
        output << "null";
    }
    output << ",\"effectiveMode\":\"" << jsonEscape(settings.effectiveMode)
           << "\",\"source\":\"" << jsonEscape(settings.configurationSource)
           << "\",\"restartRequired\":false"
           << ",\"minimumRealtimeThreshold\":" << d.minimumRealtimeSpeed
           << ",\"calibration\":{"
           << "\"profilePresent\":" << jsonBool(d.calibrationProfilePresent)
           << ",\"profileValid\":" << jsonBool(d.calibrationProfileValid)
           << ",\"softwareCalibrated\":" << jsonBool(d.softwareCalibrated)
           << ",\"softwareSuitable\":" << jsonBool(d.softwareSuitable)
           << "},\"vaapi\":{"
           << "\"implemented\":" << jsonBool(d.vaapiImplemented)
           << ",\"available\":" << jsonBool(d.vaapiAvailable)
           << ",\"calibrated\":" << jsonBool(d.vaapiCalibrated)
           << ",\"suitable\":" << jsonBool(d.vaapiSuitable)
           << ",\"forcedBelowThreshold\":" << jsonBool(d.forcedVaapiBelowThreshold)
           << ",\"reason\":\"" << jsonEscape(d.vaapiReason) << "\"}"
           << ",\"availableManagedModes\":[\"auto\",\"software\",\"vaapi\"]}";
    return output.str();
}

ApiResponse errorResponse(
    int statusCode,
    const std::string& code,
    const std::string& message)
{
    ApiResponse response;
    response.statusCode = statusCode;
    response.contentType = "application/json";
    response.headers["Cache-Control"] = "no-store";
    response.body =
        "{\"error\":{\"code\":\"" + jsonEscape(code) +
        "\",\"message\":\"" + jsonEscape(message) + "\"}}";
    return response;
}

class FlatJsonParser
{
public:
    explicit FlatJsonParser(const std::string& text) : text_(text) {}

    bool parse(
        std::map<std::string, std::string>& strings,
        std::map<std::string, bool>& booleans)
    {
        strings.clear();
        booleans.clear();
        skipWhitespace();
        if (!consume('{')) return false;
        skipWhitespace();
        if (consume('}')) {
            skipWhitespace();
            return position_ == text_.size();
        }

        while (position_ < text_.size()) {
            std::string key;
            if (!parseString(key)) return false;
            skipWhitespace();
            if (!consume(':')) return false;
            skipWhitespace();

            if (position_ < text_.size() && text_[position_] == '"') {
                std::string value;
                if (!parseString(value) || !strings.emplace(key, value).second)
                    return false;
            }
            else if (consumeLiteral("true")) {
                if (!booleans.emplace(key, true).second) return false;
            }
            else if (consumeLiteral("false")) {
                if (!booleans.emplace(key, false).second) return false;
            }
            else {
                return false;
            }

            skipWhitespace();
            if (consume('}')) {
                skipWhitespace();
                return position_ == text_.size();
            }
            if (!consume(',')) return false;
            skipWhitespace();
        }
        return false;
    }

private:
    void skipWhitespace()
    {
        while (position_ < text_.size() &&
               std::isspace(static_cast<unsigned char>(text_[position_])))
            ++position_;
    }

    bool consume(char character)
    {
        if (position_ >= text_.size() || text_[position_] != character)
            return false;
        ++position_;
        return true;
    }

    bool consumeLiteral(const char* literal)
    {
        const std::string value(literal);
        if (text_.compare(position_, value.size(), value) != 0) return false;
        position_ += value.size();
        return true;
    }

    bool parseString(std::string& value)
    {
        if (!consume('"')) return false;
        value.clear();
        while (position_ < text_.size()) {
            const unsigned char character =
                static_cast<unsigned char>(text_[position_++]);
            if (character == '"') return true;
            if (character < 0x20U) return false;
            if (character != '\\') {
                value.push_back(static_cast<char>(character));
                continue;
            }
            if (position_ >= text_.size()) return false;
            switch (text_[position_++]) {
            case '"': value.push_back('"'); break;
            case '\\': value.push_back('\\'); break;
            case '/': value.push_back('/'); break;
            case 'b': value.push_back('\b'); break;
            case 'f': value.push_back('\f'); break;
            case 'n': value.push_back('\n'); break;
            case 'r': value.push_back('\r'); break;
            case 't': value.push_back('\t'); break;
            default: return false;
            }
        }
        return false;
    }

    const std::string& text_;
    std::size_t position_ = 0;
};

bool allowedPayload(
    const std::map<std::string, std::string>& strings,
    const std::map<std::string, bool>& booleans)
{
    static const std::set<std::string> AllowedStrings = {
        "backendId", "videoEncoderMode", "operationId"
    };
    static const std::set<std::string> AllowedBooleans = {
        "clearManagedOverride"
    };
    for (const auto& entry : strings)
        if (AllowedStrings.count(entry.first) == 0U) return false;
    for (const auto& entry : booleans)
        if (AllowedBooleans.count(entry.first) == 0U) return false;
    return true;
}

std::string stringValue(
    const std::map<std::string, std::string>& values,
    const std::string& key)
{
    const auto found = values.find(key);
    return found == values.end() ? std::string() : found->second;
}

bool boolValue(
    const std::map<std::string, bool>& values,
    const std::string& key)
{
    const auto found = values.find(key);
    return found != values.end() && found->second;
}
}

MediaTranscodeSettingsApiRuntime& MediaTranscodeSettingsApiRuntime::instance()
{
    static MediaTranscodeSettingsApiRuntime runtime;
    return runtime;
}

bool MediaTranscodeSettingsApiRuntime::configure(Database& database)
{
    std::lock_guard<std::mutex> lock(mutex_);
    services_.clear();
    database_ = &database;
    return database.isOpen();
}

bool MediaTranscodeSettingsApiRuntime::configured() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return database_ != nullptr && database_->isOpen();
}

void MediaTranscodeSettingsApiRuntime::reset()
{
    std::lock_guard<std::mutex> lock(mutex_);
    services_.clear();
    database_ = nullptr;
}

MediaTranscodeBackendSettingsService*
MediaTranscodeSettingsApiRuntime::findOrCreateService(
    const std::string& backendId) const
{
    if (!MediaTranscodeBackendSettingsService::validBackendId(backendId)) {
        return nullptr;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    if (database_ == nullptr || !database_->isOpen()) return nullptr;
    const auto found = services_.find(backendId);
    if (found != services_.end()) return found->second.get();

    auto service = std::make_unique<MediaTranscodeBackendSettingsService>(
        *database_, backendId);
    if (!service->ensureSchema()) return nullptr;
    MediaTranscodeBackendSettingsService* result = service.get();
    services_.emplace(backendId, std::move(service));
    return result;
}

bool MediaTranscodeSettingsApiRuntime::tryHandleGet(
    const std::string& requestTarget,
    ApiResponse& response) const
{
    std::string backendId;
    if (!routeBackendId(requestTarget, backendId)) return false;

    MediaTranscodeBackendSettingsService* service = findOrCreateService(backendId);
    if (service == nullptr) {
        response = errorResponse(
            503,
            "backend_media_transcode_settings_unavailable",
            "Media transcode settings are unavailable for this backend");
        return true;
    }

    const MediaTranscodeBackendSettingsSnapshot settings = service->get();
    if (settings.backendId.empty()) {
        response = errorResponse(
            503,
            "backend_media_transcode_settings_unavailable",
            "Media transcode settings could not be read");
        return true;
    }

    response.statusCode = 200;
    response.contentType = "application/json";
    response.headers["Cache-Control"] = "no-store";
    response.body = serialize(settings);
    return true;
}

bool MediaTranscodeSettingsApiRuntime::tryHandlePost(
    const std::string& requestTarget,
    const std::string& body,
    ApiResponse& response) const
{
    std::string backendId;
    if (!routeBackendId(requestTarget, backendId)) return false;

    std::map<std::string, std::string> strings;
    std::map<std::string, bool> booleans;
    FlatJsonParser parser(body);
    if (!parser.parse(strings, booleans) || !allowedPayload(strings, booleans)) {
        response = errorResponse(
            400,
            "invalid_settings_payload",
            "The media transcode settings payload is invalid");
        return true;
    }

    MediaTranscodeBackendSettingsUpdate request;
    request.backendId = stringValue(strings, "backendId");
    request.videoEncoderMode = stringValue(strings, "videoEncoderMode");
    request.clearManagedOverride = boolValue(booleans, "clearManagedOverride");

    if (request.backendId != backendId) {
        response = errorResponse(
            400,
            "backend_id_mismatch",
            "The backend ID in the route and payload must match");
        return true;
    }

    MediaTranscodeBackendSettingsService* service = findOrCreateService(backendId);
    if (service == nullptr) {
        response = errorResponse(
            503,
            "backend_media_transcode_settings_unavailable",
            "Media transcode settings are unavailable for this backend");
        return true;
    }

    const MediaTranscodeBackendSettingsUpdateResult result = service->update(request);
    if (!result.success) {
        response = errorResponse(result.statusCode, result.errorCode, result.message);
        return true;
    }

    response.statusCode = 200;
    response.contentType = "application/json";
    response.headers["Cache-Control"] = "no-store";
    response.body = serialize(result.settings);
    return true;
}

MediaTranscodePolicy MediaTranscodeSettingsApiRuntime::resolvePolicy(
    const std::string& backendId) const
{
    MediaTranscodeBackendSettingsService* service = findOrCreateService(backendId);
    return service == nullptr
        ? MediaTranscodePolicy::fromEnvironment()
        : service->resolvePolicy();
}
