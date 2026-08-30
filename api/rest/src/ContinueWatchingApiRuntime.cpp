#include "ContinueWatchingApiRuntime.h"

#include "ContinueWatching.h"
#include "Database.h"
#include "VdrRecording.h"
#include "VdrRecordingArtworkIdentity.h"
#include "VdrRecordingCacheRepository.h"

#include <cctype>
#include <map>
#include <optional>
#include <sstream>
#include <string>

namespace
{
constexpr const char* Route = "/api/media/continue-watching";

std::string jsonEscape(const std::string& value)
{
    std::string escaped;
    for (const char c : value) {
        switch (c) {
        case '"': escaped += "\\\""; break;
        case '\\': escaped += "\\\\"; break;
        case '\n': escaped += "\\n"; break;
        case '\r': escaped += "\\r"; break;
        case '\t': escaped += "\\t"; break;
        default:
            if (static_cast<unsigned char>(c) >= 0x20U) escaped.push_back(c);
            break;
        }
    }
    return escaped;
}

ApiResponse errorResponse(int status, const std::string& code)
{
    ApiResponse response;
    response.statusCode = status;
    response.contentType = "application/json";
    response.headers["Cache-Control"] = "no-store";
    response.body = "{\"error\":{\"code\":\"" + jsonEscape(code) + "\"}}";
    return response;
}

class Parser
{
public:
    explicit Parser(const std::string& text) : text_(text) {}

    bool parse()
    {
        skip();
        if (!take('{')) return false;
        skip();
        if (take('}')) return position_ == text_.size();
        while (position_ < text_.size()) {
            std::string key;
            if (!string(key)) return false;
            skip(); if (!take(':')) return false; skip();
            if (position_ < text_.size() && text_[position_] == '"') {
                std::string value;
                if (!string(value) || !strings_.emplace(key, value).second) return false;
            } else if (literal("true")) {
                if (!bools_.emplace(key, true).second) return false;
            } else if (literal("false")) {
                if (!bools_.emplace(key, false).second) return false;
            } else {
                bool negative = false;
                if (position_ < text_.size() && text_[position_] == '-') {
                    negative = true; ++position_;
                }
                if (position_ >= text_.size() || !std::isdigit(static_cast<unsigned char>(text_[position_]))) return false;
                long long value = 0;
                while (position_ < text_.size() && std::isdigit(static_cast<unsigned char>(text_[position_]))) {
                    value = value * 10 + (text_[position_] - '0'); ++position_;
                }
                if (!numbers_.emplace(key, negative ? -value : value).second) return false;
            }
            skip();
            if (take('}')) { skip(); return position_ == text_.size(); }
            if (!take(',')) return false;
            skip();
        }
        return false;
    }

    std::string stringValue(const std::string& key) const
    {
        const auto it = strings_.find(key);
        return it == strings_.end() ? std::string() : it->second;
    }
    bool boolValue(const std::string& key, bool fallback = false) const
    {
        const auto it = bools_.find(key);
        return it == bools_.end() ? fallback : it->second;
    }
    long long numberValue(const std::string& key, long long fallback = 0) const
    {
        const auto it = numbers_.find(key);
        return it == numbers_.end() ? fallback : it->second;
    }

private:
    void skip() { while (position_ < text_.size() && std::isspace(static_cast<unsigned char>(text_[position_]))) ++position_; }
    bool take(char c) { if (position_ >= text_.size() || text_[position_] != c) return false; ++position_; return true; }
    bool literal(const char* value) { const std::string s(value); if (text_.compare(position_, s.size(), s) != 0) return false; position_ += s.size(); return true; }
    bool string(std::string& value)
    {
        if (!take('"')) return false;
        while (position_ < text_.size()) {
            char c = text_[position_++];
            if (c == '"') return true;
            if (c == '\\') {
                if (position_ >= text_.size()) return false;
                c = text_[position_++];
                switch (c) {
                case '"': value.push_back('"'); break;
                case '\\': value.push_back('\\'); break;
                case '/': value.push_back('/'); break;
                case 'n': value.push_back('\n'); break;
                case 'r': value.push_back('\r'); break;
                case 't': value.push_back('\t'); break;
                default: return false;
                }
            } else if (static_cast<unsigned char>(c) < 0x20U) return false;
            else value.push_back(c);
        }
        return false;
    }

    const std::string& text_;
    std::size_t position_ = 0;
    std::map<std::string, std::string> strings_;
    std::map<std::string, bool> bools_;
    std::map<std::string, long long> numbers_;
};

std::string serialize(const std::vector<ContinueWatchingItem>& items)
{
    std::ostringstream out;
    out << "{\"items\":[";
    bool first = true;
    for (const auto& item : items) {
        if (!first) out << ',';
        first = false;
        out << "{\"backendId\":\"" << jsonEscape(item.recording.backendId)
            << "\",\"recordingId\":\"" << jsonEscape(item.recording.recordingId)
            << "\",\"backendNativeId\":\"" << jsonEscape(item.recording.backendNativeId)
            << "\",\"title\":\"" << jsonEscape(item.recording.title)
            << "\",\"subtitle\":\"" << jsonEscape(item.recording.subtitle)
            << "\",\"posterUrl\":\"" << jsonEscape(item.recording.posterUrl)
            << "\",\"resumePositionSeconds\":" << item.resumePositionSeconds
            << ",\"durationKnown\":" << (item.recording.durationKnown ? "true" : "false")
            << ",\"durationSeconds\":" << (item.recording.durationKnown ? item.recording.durationSeconds : 0)
            << ",\"lastActivityAt\":\"" << jsonEscape(item.lastActivityAt) << "\"}";
    }
    out << "]}";
    return out.str();
}
}

ContinueWatchingApiRuntime& ContinueWatchingApiRuntime::instance()
{
    static ContinueWatchingApiRuntime runtime;
    return runtime;
}

bool ContinueWatchingApiRuntime::configure(
    Database& database,
    VdrRecordingCacheRepository& recordings)
{
    std::lock_guard<std::mutex> lock(mutex_);
    repository_ = std::make_unique<ContinueWatchingRepository>(database);
    if (!repository_->ensureSchema()) {
        repository_.reset();
        return false;
    }
    service_ = std::make_unique<ContinueWatchingService>(
        *repository_,
        [&recordings](const std::string& backendId, const std::string& recordingId)
            -> std::optional<ContinueWatchingRecordingTruth> {
            const auto current = recordings.findAllForBackend(backendId);
            for (const auto& recording : current) {
                if (recording.id != recordingId) continue;
                ContinueWatchingRecordingTruth truth;
                truth.backendId = recording.backendId;
                truth.recordingId = recording.id;
                truth.backendNativeId = recording.backendNativeId;
                truth.title = recording.title;
                const VdrRecordingArtworkRef* preferredArtwork =
                    VdrRecordingArtworkIdentity::preferredArtwork(recording);
                if (preferredArtwork != nullptr) {
                    truth.posterUrl = VdrRecordingArtworkIdentity::publicUrl(
                        recording,
                        *preferredArtwork);
                }
                truth.durationSeconds = recording.durationSeconds;
                truth.durationKnown = recording.recordingDurationKnown && recording.durationSeconds > 0;
                return truth;
            }
            return std::nullopt;
        });
    return true;
}

void ContinueWatchingApiRuntime::reset()
{
    std::lock_guard<std::mutex> lock(mutex_);
    service_.reset();
    repository_.reset();
}

bool ContinueWatchingApiRuntime::tryHandlePost(
    const std::string& requestTarget,
    const std::string& body,
    const std::string& actorRef,
    ApiResponse& response) const
{
    const std::size_t query = requestTarget.find('?');
    const std::string path = query == std::string::npos ? requestTarget : requestTarget.substr(0, query);
    if (path != Route) return false;
    if (actorRef.empty()) {
        response = errorResponse(401, "continue_watching_actor_required");
        return true;
    }

    Parser parser(body);
    if (!parser.parse()) {
        response = errorResponse(400, "invalid_continue_watching_payload");
        return true;
    }
    const std::string operation = parser.stringValue("operation");
    const std::string backendId = parser.stringValue("backendId");
    if (backendId.empty()) {
        response = errorResponse(400, "continue_watching_backend_required");
        return true;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    if (!service_) {
        response = errorResponse(503, "continue_watching_unavailable");
        return true;
    }

    if (operation == "list") {
        response.statusCode = 200;
        response.contentType = "application/json";
        response.headers["Cache-Control"] = "no-store";
        response.body = serialize(service_->list(actorRef, backendId));
        return true;
    }

    const std::string recordingId = parser.stringValue("recordingId");
    const std::string operationId = parser.stringValue("operationId");
    if (recordingId.empty() || operationId.empty()) {
        response = errorResponse(400, "continue_watching_recording_operation_required");
        return true;
    }

    bool success = false;
    if (operation == "progress") {
        const long long position = parser.numberValue("positionSeconds", -1);
        if (position < 0 || position > 2147483647LL) {
            response = errorResponse(400, "continue_watching_position_invalid");
            return true;
        }
        success = service_->recordProgress(
            actorRef,
            backendId,
            recordingId,
            static_cast<int>(position),
            parser.boolValue("resumeSupported", false),
            operationId);
    } else if (operation == "clear") {
        success = service_->clear(actorRef, backendId, recordingId, operationId);
    } else {
        response = errorResponse(400, "continue_watching_operation_invalid");
        return true;
    }

    if (!success) {
        response = errorResponse(503, "continue_watching_write_failed");
        return true;
    }
    response.statusCode = 200;
    response.contentType = "application/json";
    response.headers["Cache-Control"] = "no-store";
    response.body = "{\"ok\":true}";
    return true;
}
