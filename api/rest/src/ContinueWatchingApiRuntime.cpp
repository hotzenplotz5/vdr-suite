#include "ContinueWatchingApiRuntime.h"

#include "ContinueWatching.h"
#include "ManualRecordingMetadataApiRuntime.h"
#include "Database.h"
#include "RecentlyWatched.h"
#include "VdrRecording.h"
#include "VdrRecordingArtworkIdentity.h"
#include "VdrRecordingCacheRepository.h"
#include "VdrRecordingMetadataJsonSerializer.h"

#include <cctype>
#include <climits>
#include <map>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

namespace
{
constexpr const char* ContinueWatchingRoute = "/api/media/continue-watching";
constexpr const char* RecentlyWatchedRoute = "/api/media/recently-watched";

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
                    const int digit = text_[position_] - '0';
                    if (value > (LLONG_MAX - digit) / 10) return false;
                    value = value * 10 + digit; ++position_;
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

    bool exactPublicProgressFields() const
    {
        return (strings_.size() == 3U || strings_.size() == 4U) &&
            numbers_.size() == 2U && bools_.empty() &&
            strings_.count("backendId") == 1U &&
            strings_.count("recordingId") == 1U &&
            strings_.count("operationId") == 1U &&
            (strings_.size() == 3U || strings_.count("sessionId") == 1U) &&
            numbers_.count("positionSeconds") == 1U &&
            numbers_.count("expectedRevision") == 1U;
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
    bool hasNumber(const std::string& key) const
    {
        return numbers_.count(key) != 0U;
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

const VdrRecording* findRecording(
    const std::vector<VdrRecording>& recordings,
    const std::string& backendId,
    const std::string& recordingId)
{
    for (const auto& recording : recordings) {
        if (recording.backendId == backendId && recording.id == recordingId) return &recording;
    }
    return nullptr;
}

using ManualAssignmentMap =
    std::map<std::string, ManualRecordingMetadataAssignment>;

const ManualRecordingMetadataAssignment*
manualMetadataProjectionForRecording(
    const VdrRecording& recording,
    const ManualAssignmentMap& assignments)
{
    auto find = [&assignments](
        const std::string& key)
        -> const ManualRecordingMetadataAssignment*
    {
        if (key.empty()) return nullptr;
        const auto it = assignments.find(key);
        return it == assignments.end()
            ? nullptr
            : &it->second;
    };

    if (const auto* assignment = find(recording.backendNativeId))
        return assignment;

    if (const auto* assignment = find(recording.path))
        return assignment;

    return find(recording.id);
}

std::string manualMetadataUrlEncode(
    const std::string& value)
{
    static const char hex[] = "0123456789ABCDEF";
    std::string encoded;

    for (const unsigned char c : value)
    {
        if (std::isalnum(c) ||
            c == '-' ||
            c == '_' ||
            c == '.' ||
            c == '~')
        {
            encoded.push_back(static_cast<char>(c));
            continue;
        }

        encoded.push_back('%');
        encoded.push_back(hex[(c >> 4U) & 0x0FU]);
        encoded.push_back(hex[c & 0x0FU]);
    }

    return encoded;
}

std::string manualMetadataPosterUrl(
    const VdrRecording& recording,
    const ManualRecordingMetadataAssignment& assignment)
{
    if (!assignment.found ||
        !assignment.relationshipLocked ||
        assignment.posterReference.empty() ||
        assignment.revision <= 0 ||
        recording.backendNativeId.empty())
    {
        return {};
    }

    const std::string backendId =
        recording.backendId.empty()
            ? "default"
            : recording.backendId;

    return
        "/api/vdr/recordings/metadata/image?backend=" +
        manualMetadataUrlEncode(backendId) +
        "&backendNativeId=" +
        manualMetadataUrlEncode(recording.backendNativeId) +
        "&kind=preferred&index=0&assignmentRevision=" +
        std::to_string(assignment.revision);
}

std::string serializeRecording(const VdrRecording& recording)
{
    std::ostringstream out;
    out << "{\"id\":\"" << jsonEscape(recording.id)
        << "\",\"recordingId\":\"" << jsonEscape(recording.id)
        << "\",\"backendId\":\"" << jsonEscape(recording.backendId)
        << "\",\"title\":\"" << jsonEscape(recording.title)
        << "\",\"path\":\"" << jsonEscape(recording.path)
        << "\",\"recordingPath\":\"" << jsonEscape(recording.path)
        << "\",\"backendNativeId\":\"" << jsonEscape(recording.backendNativeId)
        << "\",\"startTime\":\"" << jsonEscape(recording.startTime)
        << "\",\"durationSeconds\":" << recording.durationSeconds
        << ",\"sizeMb\":" << recording.sizeMb
        << ",\"metadata\":" << VdrRecordingMetadataJsonSerializer::serialize(recording)
        << "}";
    return out.str();
}

std::string serializeContinueWatching(
    const std::vector<ContinueWatchingItem>& items,
    const std::vector<VdrRecording>& recordings)
{
    const std::string backendId =
        !recordings.empty()
            ? recordings.front().backendId
            : (!items.empty()
                ? items.front().recording.backendId
                : "default");

    const ManualAssignmentMap manualAssignments =
        ManualRecordingMetadataApiRuntime::instance()
            .findSelectedForBackend(backendId);
    std::ostringstream out;
    out << "{\"items\":[";
    bool first = true;
    for (const auto& item : items) {
        const VdrRecording* currentRecording = findRecording(
            recordings, item.recording.backendId, item.recording.recordingId);
        if (currentRecording == nullptr) continue;

        const ManualRecordingMetadataAssignment* manual =
            manualMetadataProjectionForRecording(
                *currentRecording,
                manualAssignments);

        const std::string currentTitle =
            VdrRecordingMetadataJsonSerializer::presentationTitle(
                *currentRecording);
        const std::string currentSubtitle =
            VdrRecordingMetadataJsonSerializer::presentationSubtitle(
                *currentRecording);
        const std::string currentPoster =
            VdrRecordingMetadataJsonSerializer::preferredArtworkUrl(
                *currentRecording);

        const std::string projectedTitle =
            manual != nullptr &&
            manual->relationshipLocked &&
            !manual->title.empty()
                ? manual->title
                : (!currentTitle.empty()
                    ? currentTitle
                    : item.recording.title);

        const std::string manualPoster =
            manual != nullptr
                ? manualMetadataPosterUrl(
                    *currentRecording,
                    *manual)
                : std::string{};

        const std::string projectedPoster =
            !manualPoster.empty()
                ? manualPoster
                : (!currentPoster.empty()
                    ? currentPoster
                    : item.recording.posterUrl);

        const std::string projectedSubtitle =
            !currentSubtitle.empty()
                ? currentSubtitle
                : item.recording.subtitle;

        if (!first) out << ',';
        first = false;
        out << "{\"backendId\":\"" << jsonEscape(item.recording.backendId)
            << "\",\"recordingId\":\"" << jsonEscape(item.recording.recordingId)
            << "\",\"backendNativeId\":\"" << jsonEscape(item.recording.backendNativeId)
            << "\",\"title\":\"" << jsonEscape(projectedTitle)
            << "\",\"subtitle\":\"" << jsonEscape(projectedSubtitle)
            << "\",\"posterUrl\":\"" << jsonEscape(projectedPoster)
            << "\",\"resumePositionSeconds\":" << item.resumePositionSeconds
            << ",\"progressRevision\":" << item.revision
            << ",\"durationKnown\":" << (item.recording.durationKnown ? "true" : "false")
            << ",\"durationSeconds\":" << (item.recording.durationKnown ? item.recording.durationSeconds : 0)
            << ",\"lastActivityAt\":\"" << jsonEscape(item.lastActivityAt)
            << "\",\"recording\":" << serializeRecording(*currentRecording)
            << "}";
    }
    out << "]}";
    return out.str();
}

std::string serializeRecentlyWatched(
    const std::vector<RecentlyWatchedItem>& items,
    const std::vector<VdrRecording>& recordings)
{
    const std::string backendId =
        !recordings.empty()
            ? recordings.front().backendId
            : (!items.empty()
                ? items.front().recording.backendId
                : "default");

    const ManualAssignmentMap manualAssignments =
        ManualRecordingMetadataApiRuntime::instance()
            .findSelectedForBackend(backendId);
    std::ostringstream out;
    out << "{\"items\":[";
    bool first = true;
    for (const auto& item : items) {
        const VdrRecording* currentRecording = findRecording(
            recordings, item.recording.backendId, item.recording.recordingId);
        if (currentRecording == nullptr) continue;

        const ManualRecordingMetadataAssignment* manual =
            manualMetadataProjectionForRecording(
                *currentRecording,
                manualAssignments);

        const std::string currentTitle =
            VdrRecordingMetadataJsonSerializer::presentationTitle(
                *currentRecording);
        const std::string currentSubtitle =
            VdrRecordingMetadataJsonSerializer::presentationSubtitle(
                *currentRecording);
        const std::string currentPoster =
            VdrRecordingMetadataJsonSerializer::preferredArtworkUrl(
                *currentRecording);

        const std::string projectedTitle =
            manual != nullptr &&
            manual->relationshipLocked &&
            !manual->title.empty()
                ? manual->title
                : (!currentTitle.empty()
                    ? currentTitle
                    : item.recording.title);

        const std::string manualPoster =
            manual != nullptr
                ? manualMetadataPosterUrl(
                    *currentRecording,
                    *manual)
                : std::string{};

        const std::string projectedPoster =
            !manualPoster.empty()
                ? manualPoster
                : (!currentPoster.empty()
                    ? currentPoster
                    : item.recording.posterUrl);

        const std::string projectedSubtitle =
            !currentSubtitle.empty()
                ? currentSubtitle
                : item.recording.subtitle;

        if (!first) out << ',';
        first = false;
        out << "{\"backendId\":\"" << jsonEscape(item.recording.backendId)
            << "\",\"recordingId\":\"" << jsonEscape(item.recording.recordingId)
            << "\",\"backendNativeId\":\"" << jsonEscape(item.recording.backendNativeId)
            << "\",\"title\":\"" << jsonEscape(projectedTitle)
            << "\",\"subtitle\":\"" << jsonEscape(projectedSubtitle)
            << "\",\"posterUrl\":\"" << jsonEscape(projectedPoster)
            << "\",\"positionKnown\":" << (item.positionKnown ? "true" : "false")
            << ",\"positionSeconds\":" << item.positionSeconds
            << ",\"completionKnown\":" << (item.completionKnown ? "true" : "false")
            << ",\"completed\":" << (item.completed ? "true" : "false")
            << ",\"resumeRelevanceKnown\":" << (item.resumeRelevanceKnown ? "true" : "false")
            << ",\"resumeRelevant\":" << (item.resumeRelevant ? "true" : "false")
            << ",\"sourceEvidence\":\"" << jsonEscape(item.sourceEvidence)
            << "\",\"lastActivityAt\":\"" << jsonEscape(item.lastActivityAt)
            << "\",\"recording\":" << serializeRecording(*currentRecording)
            << "}";
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
    recentlyWatchedRepository_ = std::make_unique<RecentlyWatchedRepository>(database);
    if (!repository_->ensureSchema() || !recentlyWatchedRepository_->ensureSchema()) {
        repository_.reset();
        service_.reset();
        recentlyWatchedRepository_.reset();
        recentlyWatchedService_.reset();
        recordings_ = nullptr;
        return false;
    }
    auto continueTruth = [](const VdrRecording& recording) {
        ContinueWatchingRecordingTruth truth;
        truth.backendId = recording.backendId;
        truth.recordingId = recording.id;
        truth.backendNativeId = recording.backendNativeId;
        truth.title =
            VdrRecordingMetadataJsonSerializer::presentationTitle(recording);
        truth.subtitle =
            VdrRecordingMetadataJsonSerializer::presentationSubtitle(recording);
        truth.posterUrl =
            VdrRecordingMetadataJsonSerializer::preferredArtworkUrl(recording);
        truth.durationSeconds = recording.durationSeconds;
        truth.durationKnown =
            recording.recordingDurationKnown && recording.durationSeconds > 0;
        return truth;
    };
    auto recentTruth = [](const VdrRecording& recording) {
        RecentlyWatchedRecordingTruth truth;
        truth.backendId = recording.backendId;
        truth.recordingId = recording.id;
        truth.backendNativeId = recording.backendNativeId;
        truth.title =
            VdrRecordingMetadataJsonSerializer::presentationTitle(recording);
        truth.subtitle =
            VdrRecordingMetadataJsonSerializer::presentationSubtitle(recording);
        truth.posterUrl =
            VdrRecordingMetadataJsonSerializer::preferredArtworkUrl(recording);
        truth.durationSeconds = recording.durationSeconds;
        truth.durationKnown =
            recording.recordingDurationKnown && recording.durationSeconds > 0;
        return truth;
    };

    service_ = std::make_unique<ContinueWatchingService>(
        *repository_,
        [&recordings, continueTruth](const std::string& backendId, const std::string& recordingId)
            -> std::optional<ContinueWatchingRecordingTruth> {
            const auto current = recordings.findAllForBackend(backendId);
            for (const auto& recording : current) {
                if (recording.id == recordingId) return continueTruth(recording);
            }
            return std::nullopt;
        },
        [&recordings, continueTruth](const std::string& backendId, const std::string& backendNativeId)
            -> std::optional<ContinueWatchingRecordingTruth> {
            const auto current = recordings.findAllForBackend(backendId);
            for (const auto& recording : current) {
                if (recording.backendNativeId == backendNativeId)
                    return continueTruth(recording);
            }
            return std::nullopt;
        });

    recentlyWatchedService_ = std::make_unique<RecentlyWatchedService>(
        *recentlyWatchedRepository_,
        [&recordings, recentTruth](const std::string& backendId, const std::string& recordingId)
            -> std::optional<RecentlyWatchedRecordingTruth> {
            const auto current = recordings.findAllForBackend(backendId);
            for (const auto& recording : current) {
                if (recording.id == recordingId) return recentTruth(recording);
            }
            return std::nullopt;
        },
        [&recordings, recentTruth](const std::string& backendId, const std::string& backendNativeId)
            -> std::optional<RecentlyWatchedRecordingTruth> {
            const auto current = recordings.findAllForBackend(backendId);
            for (const auto& recording : current) {
                if (recording.backendNativeId == backendNativeId)
                    return recentTruth(recording);
            }
            return std::nullopt;
        });
    recordings_ = &recordings;
    return true;
}

void ContinueWatchingApiRuntime::reset()
{
    std::lock_guard<std::mutex> lock(mutex_);
    recentlyWatchedService_.reset();
    recentlyWatchedRepository_.reset();
    service_.reset();
    repository_.reset();
    recordings_ = nullptr;
}


PublicRecordingProgressRequest ContinueWatchingApiRuntime::parsePublicProgressRequest(
    const std::string& body)
{
    PublicRecordingProgressRequest result;
    if (body.size() > 2048U) return result;
    Parser parser(body);
    if (!parser.parse() || !parser.exactPublicProgressFields()) return result;
    result.backendId = parser.stringValue("backendId");
    result.recordingId = parser.stringValue("recordingId");
    result.operationId = parser.stringValue("operationId");
    result.sessionId = parser.stringValue("sessionId");
    const long long position = parser.numberValue("positionSeconds", -1);
    const long long revision = parser.numberValue("expectedRevision", -1);
    if (result.backendId.empty() || result.recordingId.empty() ||
        result.operationId.empty() || result.operationId.size() > 128U ||
        position < 0 || position > INT_MAX || revision < 0 ||
        (position > 0 && result.sessionId.empty()) ||
        result.sessionId.size() > 128U)
        return result;
    for (unsigned char ch : result.operationId) {
        if (ch < 0x21U || ch > 0x7eU) return result;
    }
    for (unsigned char ch : result.sessionId) {
        if (ch < 0x21U || ch > 0x7eU) return result;
    }
    result.positionSeconds = static_cast<int>(position);
    result.expectedRevision = revision;
    result.valid = true;
    return result;
}

std::vector<ContinueWatchingItem> ContinueWatchingApiRuntime::publicCandidates(
    const std::string& actorId, const std::string& backendId) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (!service_ || actorId.empty() || backendId.empty())
        return {};
    // Existing Service resolves current Recording truth, prunes missing
    // entries and retains actor/backend isolation. Public projection and
    // per-recording admission remain in the Device HTTP owner.
    return service_->list(actorId, backendId);
}

ApiResponse ContinueWatchingApiRuntime::publicProgressGet(
    const std::string& actorId,
    const VdrRecording& admittedRecording,
    const std::string& publicRecordingId) const
{
    if (actorId.empty() || admittedRecording.id.empty() ||
        admittedRecording.backendNativeId.empty() ||
        publicRecordingId.rfind("rec_", 0) != 0)
        return errorResponse(400, "recording_progress_invalid_scope");
    const std::string backendId = admittedRecording.backendId.empty()
        ? "default" : admittedRecording.backendId;

    std::lock_guard<std::mutex> lock(mutex_);
    if (!repository_) return errorResponse(503, "recording_progress_unavailable");
    const auto states = repository_->findForActorBackend(actorId, backendId);
    std::int64_t revision = 0;
    int position = 0;
    for (const auto& state : states) {
        if (state.backendNativeId == admittedRecording.backendNativeId) {
            // Recording list/index ID may change while native identity stays.
            // Do not lose the revision fence on a canonical rec_ rebind.
            revision = state.revision;
            position = state.positionSeconds;
            break;
        }
    }
    ApiResponse response;
    response.statusCode = 200;
    response.contentType = "application/json";
    response.headers["Cache-Control"] = "no-store";
    response.headers["X-Content-Type-Options"] = "nosniff";
    response.body = "{\"backendId\":\"" + jsonEscape(backendId) +
        "\",\"recordingId\":\"" + jsonEscape(publicRecordingId) +
        "\",\"hasProgress\":" + (position > 0 ? "true" : "false") +
        ",\"positionSeconds\":" + std::to_string(position) +
        ",\"revision\":" + std::to_string(revision) +
        ",\"durationSeconds\":";
    if (admittedRecording.recordingDurationKnown &&
        admittedRecording.durationSeconds > 0)
        response.body += std::to_string(admittedRecording.durationSeconds);
    else response.body += "null";
    // Actual start-at support belongs to the selected MediaSession profile.
    response.body += ",\"resumeSupport\":\"profile_dependent\"}";
    return response;
}

ApiResponse ContinueWatchingApiRuntime::publicProgressUpdate(
    const std::string& actorId,
    const VdrRecording& admittedRecording,
    const std::string& publicRecordingId,
    const PublicRecordingProgressRequest& request) const
{
    if (!request.valid || actorId.empty() ||
        admittedRecording.id.empty() || admittedRecording.backendNativeId.empty() ||
        publicRecordingId != request.recordingId ||
        publicRecordingId.rfind("rec_", 0) != 0)
        return errorResponse(400, "recording_progress_invalid_request");
    const std::string backendId = admittedRecording.backendId.empty()
        ? "default" : admittedRecording.backendId;
    if (request.backendId != backendId)
        return errorResponse(400, "recording_progress_backend_mismatch");

    // A client reports the last *observed* playback position. Neither wall
    // time nor an issued MediaSession is playback evidence. Exact completion
    // clears Resume without inventing a near-end threshold.
    int position = request.positionSeconds;
    if (admittedRecording.recordingDurationKnown &&
        admittedRecording.durationSeconds > 0 &&
        position >= admittedRecording.durationSeconds)
        position = 0;

    std::lock_guard<std::mutex> lock(mutex_);
    if (!repository_) return errorResponse(503, "recording_progress_unavailable");
    const auto mutation = repository_->updateConditional(
        actorId, backendId, admittedRecording.id,
        admittedRecording.backendNativeId, position,
        request.operationId, request.expectedRevision);
    if (mutation.status == ContinueWatchingMutationStatus::conflict) {
        ApiResponse response = errorResponse(409, "recording_progress_revision_conflict");
        response.body = "{\"error\":{\"code\":\"recording_progress_revision_conflict\"},"
            "\"currentRevision\":" + std::to_string(mutation.revision) + "}";
        return response;
    }
    if (mutation.status == ContinueWatchingMutationStatus::invalid)
        return errorResponse(400, "recording_progress_invalid_request");
    if (mutation.status == ContinueWatchingMutationStatus::unavailable)
        return errorResponse(503, "recording_progress_unavailable");

    ApiResponse response;
    response.statusCode = 200;
    response.contentType = "application/json";
    response.headers["Cache-Control"] = "no-store";
    response.headers["X-Content-Type-Options"] = "nosniff";
    response.body = "{\"backendId\":\"" + jsonEscape(backendId) +
        "\",\"recordingId\":\"" + jsonEscape(publicRecordingId) +
        "\",\"positionSeconds\":" + std::to_string(position) +
        ",\"hasProgress\":" + (position > 0 ? "true" : "false") +
        ",\"revision\":" + std::to_string(mutation.revision) +
        ",\"replayed\":" +
        (mutation.status == ContinueWatchingMutationStatus::replayed
             ? "true" : "false") + "}";
    return response;
}

bool ContinueWatchingApiRuntime::tryHandlePost(
    const std::string& requestTarget,
    const std::string& body,
    const std::string& actorRef,
    ApiResponse& response) const
{
    const std::size_t query = requestTarget.find('?');
    const std::string path = query == std::string::npos ? requestTarget : requestTarget.substr(0, query);
    const bool recentlyWatched = path == RecentlyWatchedRoute;
    if (path != ContinueWatchingRoute && !recentlyWatched) return false;
    if (actorRef.empty()) {
        response = errorResponse(401, recentlyWatched
            ? "recently_watched_actor_required"
            : "continue_watching_actor_required");
        return true;
    }

    Parser parser(body);
    if (!parser.parse()) {
        response = errorResponse(400, recentlyWatched
            ? "invalid_recently_watched_payload"
            : "invalid_continue_watching_payload");
        return true;
    }
    const std::string operation = parser.stringValue("operation");
    const std::string backendId = parser.stringValue("backendId");
    if (backendId.empty()) {
        response = errorResponse(400, recentlyWatched
            ? "recently_watched_backend_required"
            : "continue_watching_backend_required");
        return true;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    if (!recordings_ || (recentlyWatched ? !recentlyWatchedService_ : !service_)) {
        response = errorResponse(503, recentlyWatched
            ? "recently_watched_unavailable"
            : "continue_watching_unavailable");
        return true;
    }

    if (recentlyWatched) {
        if (operation == "list") {
            const auto items = recentlyWatchedService_->list(actorRef, backendId);
            const auto currentRecordings = recordings_->findAllForBackend(backendId);
            response.statusCode = 200;
            response.contentType = "application/json";
            response.headers["Cache-Control"] = "no-store";
            response.body = serializeRecentlyWatched(items, currentRecordings);
            return true;
        }
        if (operation != "activity") {
            response = errorResponse(400, "recently_watched_operation_invalid");
            return true;
        }
        const std::string recordingId = parser.stringValue("recordingId");
        const std::string operationId = parser.stringValue("operationId");
        const long long position = parser.numberValue("positionSeconds", -1);
        if (recordingId.empty() || operationId.empty() || position < -1 || position > INT_MAX) {
            response = errorResponse(400, "recently_watched_activity_invalid");
            return true;
        }
        const bool success = recentlyWatchedService_->recordActivity(
            actorRef,
            backendId,
            recordingId,
            position < 0 ? 0 : static_cast<int>(position),
            position >= 0,
            parser.boolValue("resumeSupportKnown", false),
            parser.boolValue("resumeSupported", false),
            parser.boolValue("ended", false),
            operationId);
        if (!success) {
            response = errorResponse(503, "recently_watched_write_failed");
            return true;
        }
        response.statusCode = 200;
        response.contentType = "application/json";
        response.headers["Cache-Control"] = "no-store";
        response.body = "{\"ok\":true}";
        return true;
    }

    // P1 Browser CAS handshake: reveal only the current actor's revision
    // for the exact existing internal Recording. The native key never leaves
    // Suite. A zero-position tombstone must remain observable so stale
    // checkpoints cannot resurrect it.
    if (operation == "revision") {
        const std::string requestedId=parser.stringValue("recordingId");
        if (requestedId.empty() || requestedId.size()>512U) {
            response=errorResponse(400,"continue_watching_recording_required");
            return true;
        }
        const auto current=recordings_->findAllForBackend(backendId);
        const VdrRecording* recording=findRecording(current,backendId,requestedId);
        if (!recording || recording->backendNativeId.empty()) {
            response=errorResponse(404,"continue_watching_recording_not_found");
            return true;
        }
        const auto states=repository_->findForActorBackend(actorRef,backendId);
        std::int64_t revision=0;
        for (const auto& state:states) {
            if (state.backendNativeId==recording->backendNativeId &&
                state.revision>0) {
                revision=state.revision;
                break;
            }
        }
        response.statusCode=200;
        response.contentType="application/json";
        response.headers["Cache-Control"]="no-store";
        response.body="{\"revision\":"+std::to_string(revision)+"}";
        return true;
    }

    if (operation == "list") {
        const auto items = service_->list(actorRef, backendId);
        const auto currentRecordings = recordings_->findAllForBackend(backendId);
        response.statusCode = 200;
        response.contentType = "application/json";
        response.headers["Cache-Control"] = "no-store";
        response.body = serializeContinueWatching(items, currentRecordings);
        return true;
    }

    const std::string recordingId = parser.stringValue("recordingId");
    const std::string operationId = parser.stringValue("operationId");
    if (recordingId.empty() || operationId.empty()) {
        response = errorResponse(400, "continue_watching_recording_operation_required");
        return true;
    }

    // P1 opt-in CAS. The legacy protocol remains backwards-compatible;
    // the current Browser frontend opts in, so a late Browser pause/stop
    // cannot blindly overwrite a newer linked Device playback checkpoint.
    if (parser.hasNumber("expectedRevision")) {
        const long long expected=parser.numberValue("expectedRevision",-1);
        if (expected<0) {
            response=errorResponse(400,"continue_watching_revision_invalid");
            return true;
        }
        ContinueWatchingMutationResult updated;
        if (operation=="progress") {
            const long long position=parser.numberValue("positionSeconds",-1);
            if (position<0 || position>INT_MAX) {
                response=errorResponse(400,"continue_watching_position_invalid");
                return true;
            }
            updated=service_->recordProgressConditional(
                actorRef,backendId,recordingId,static_cast<int>(position),
                parser.boolValue("resumeSupported",false),operationId,expected);
        } else if (operation=="clear") {
            updated=service_->clearConditional(
                actorRef,backendId,recordingId,operationId,expected);
        } else {
            response=errorResponse(400,"continue_watching_operation_invalid");
            return true;
        }
        using Status=ContinueWatchingMutationStatus;
        if(updated.status==Status::conflict) {
            response=errorResponse(409,"continue_watching_revision_conflict");
            response.body="{\"error\":{\"code\":\"continue_watching_revision_conflict\"},"
                "\"currentRevision\":"+std::to_string(updated.revision)+"}";
            return true;
        }
        if(updated.status==Status::invalid) {
            response=errorResponse(400,"continue_watching_invalid_request");
            return true;
        }
        if(updated.status==Status::unavailable) {
            response=errorResponse(503,"continue_watching_write_failed");
            return true;
        }
        response.statusCode=200;
        response.contentType="application/json";
        response.headers["Cache-Control"]="no-store";
        response.body="{\"ok\":true,\"revision\":"+
            std::to_string(updated.revision)+
            ",\"replayed\":"+(updated.status==Status::replayed?"true":"false")+"}";
        return true;
    }

    bool success = false;
    if (operation == "progress") {
        const long long position = parser.numberValue("positionSeconds", -1);
        if (position < 0 || position > INT_MAX) {
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
