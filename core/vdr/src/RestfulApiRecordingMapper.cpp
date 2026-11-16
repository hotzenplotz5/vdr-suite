#include "RestfulApiRecordingMapper.h"

#include "JsonStringDecoder.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <ctime>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace {

std::size_t skipWhitespace(const std::string& text, std::size_t pos)
{
    while (pos < text.size() && std::isspace(static_cast<unsigned char>(text[pos]))) {
        ++pos;
    }
    return pos;
}

std::size_t findMatching(const std::string& text, std::size_t start, char openChar, char closeChar)
{
    bool inString = false;
    bool escaped = false;
    int depth = 0;

    for (std::size_t i = start; i < text.size(); ++i) {
        char c = text[i];

        if (inString) {
            if (escaped) {
                escaped = false;
            } else if (c == '\\') {
                escaped = true;
            } else if (c == '"') {
                inString = false;
            }
            continue;
        }

        if (c == '"') {
            inString = true;
            continue;
        }

        if (c == openChar) {
            ++depth;
        } else if (c == closeChar) {
            --depth;
            if (depth == 0) {
                return i;
            }
        }
    }

    return std::string::npos;
}

std::vector<std::string> splitTopLevelObjects(const std::string& arrayText)
{
    std::vector<std::string> objects;
    std::size_t pos = 0;

    while (pos < arrayText.size()) {
        std::size_t start = arrayText.find('{', pos);
        if (start == std::string::npos) {
            break;
        }

        std::size_t end = findMatching(arrayText, start, '{', '}');
        if (end == std::string::npos) {
            break;
        }

        objects.push_back(arrayText.substr(start, end - start + 1));
        pos = end + 1;
    }

    return objects;
}

std::string unescapeJsonString(const std::string& value)
{
    return vdrsuite::decodeJsonStringEscapes(value);
}

std::string getStringField(const std::string& objectText, const std::string& fieldName)
{
    const std::string key = "\"" + fieldName + "\"";
    std::size_t keyPos = objectText.find(key);
    if (keyPos == std::string::npos) {
        return "";
    }

    std::size_t colon = objectText.find(':', keyPos + key.size());
    if (colon == std::string::npos) {
        return "";
    }

    std::size_t quoteStart = objectText.find('"', colon + 1);
    if (quoteStart == std::string::npos) {
        return "";
    }

    bool escaped = false;
    for (std::size_t i = quoteStart + 1; i < objectText.size(); ++i) {
        char c = objectText[i];

        if (escaped) {
            escaped = false;
            continue;
        }

        if (c == '\\') {
            escaped = true;
            continue;
        }

        if (c == '"') {
            return unescapeJsonString(objectText.substr(quoteStart + 1, i - quoteStart - 1));
        }
    }

    return "";
}

int getIntField(const std::string& objectText, const std::string& fieldName, int fallback = 0)
{
    const std::string key = "\"" + fieldName + "\"";
    std::size_t keyPos = objectText.find(key);
    if (keyPos == std::string::npos) {
        return fallback;
    }

    std::size_t colon = objectText.find(':', keyPos + key.size());
    if (colon == std::string::npos) {
        return fallback;
    }

    std::size_t pos = skipWhitespace(objectText, colon + 1);
    if (pos >= objectText.size()) {
        return fallback;
    }

    bool negative = false;
    if (objectText[pos] == '-') {
        negative = true;
        ++pos;
    }

    if (pos >= objectText.size() || !std::isdigit(static_cast<unsigned char>(objectText[pos]))) {
        return fallback;
    }

    int value = 0;
    while (pos < objectText.size() && std::isdigit(static_cast<unsigned char>(objectText[pos]))) {
        value = value * 10 + (objectText[pos] - '0');
        ++pos;
    }

    return negative ? -value : value;
}

std::string normalizeRecordingName(const std::string& name)
{
    std::string normalized = name;

    std::replace(
        normalized.begin(),
        normalized.end(),
        '~',
        '/');

    return normalized;
}

std::string normalizePersonName(const std::string& name)
{
    std::string normalized;
    bool previousWasSeparator = false;

    for (char c : name) {
        unsigned char uc = static_cast<unsigned char>(c);

        if (std::isalnum(uc)) {
            normalized.push_back(
                static_cast<char>(std::tolower(uc)));
            previousWasSeparator = false;
        } else if (!previousWasSeparator && !normalized.empty()) {
            normalized.push_back('-');
            previousWasSeparator = true;
        }
    }

    while (!normalized.empty() && normalized.back() == '-') {
        normalized.pop_back();
    }

    return normalized;
}

std::string getObjectField(
    const std::string& objectText,
    const std::string& fieldName)
{
    const std::string key = "\"" + fieldName + "\"";
    std::size_t keyPos = objectText.find(key);
    if (keyPos == std::string::npos) {
        return "";
    }

    std::size_t colon = objectText.find(':', keyPos + key.size());
    if (colon == std::string::npos) {
        return "";
    }

    std::size_t objectStart = objectText.find('{', colon + 1);
    if (objectStart == std::string::npos) {
        return "";
    }

    std::size_t objectEnd = findMatching(objectText, objectStart, '{', '}');
    if (objectEnd == std::string::npos) {
        return "";
    }

    return objectText.substr(objectStart, objectEnd - objectStart + 1);
}

std::string getArrayField(
    const std::string& objectText,
    const std::string& fieldName)
{
    const std::string key = "\"" + fieldName + "\"";
    std::size_t keyPos = objectText.find(key);
    if (keyPos == std::string::npos) {
        return "";
    }

    std::size_t colon = objectText.find(':', keyPos + key.size());
    if (colon == std::string::npos) {
        return "";
    }

    std::size_t arrayStart = objectText.find('[', colon + 1);
    if (arrayStart == std::string::npos) {
        return "";
    }

    std::size_t arrayEnd = findMatching(objectText, arrayStart, '[', ']');
    if (arrayEnd == std::string::npos) {
        return "";
    }

    return objectText.substr(arrayStart + 1, arrayEnd - arrayStart - 1);
}

PersonCollection parseAdditionalMediaActors(
    const std::string& objectText)
{
    PersonCollection persons =
        PersonCollection::createEmpty();

    const std::string additionalMedia =
        getObjectField(objectText, "additional_media");

    if (additionalMedia.empty()) {
        return persons;
    }

    const std::string actorsArray =
        getArrayField(additionalMedia, "actors");

    if (actorsArray.empty()) {
        return persons;
    }

    const std::vector<std::string> actorObjects =
        splitTopLevelObjects(actorsArray);

    for (const std::string& actorObject : actorObjects) {
        const std::string name =
            getStringField(actorObject, "name");

        if (name.empty()) {
            continue;
        }

        const std::string characterName =
            getStringField(actorObject, "role");

        persons.add(
            Person::withCharacterName(
                ContentClassificationSource::Tvscraper,
                PersonRole::Actor,
                name,
                normalizePersonName(name),
                characterName));
    }

    return persons;
}

long long getLongLongField(const std::string& objectText, const std::string& fieldName, long long fallback = 0)
{
    const std::string key = "\"" + fieldName + "\"";
    std::size_t keyPos = objectText.find(key);
    if (keyPos == std::string::npos) {
        return fallback;
    }

    std::size_t colon = objectText.find(':', keyPos + key.size());
    if (colon == std::string::npos) {
        return fallback;
    }

    std::size_t pos = skipWhitespace(objectText, colon + 1);
    if (pos >= objectText.size()) {
        return fallback;
    }

    bool negative = false;
    if (objectText[pos] == '-') {
        negative = true;
        ++pos;
    }

    if (pos >= objectText.size() || !std::isdigit(static_cast<unsigned char>(objectText[pos]))) {
        return fallback;
    }

    long long value = 0;
    while (pos < objectText.size() && std::isdigit(static_cast<unsigned char>(objectText[pos]))) {
        value = value * 10 + (objectText[pos] - '0');
        ++pos;
    }

    return negative ? -value : value;
}

long long deriveRecordingStartTimeFromPath(const std::string& path)
{
    std::string normalizedPath = path;

    std::replace(
        normalizedPath.begin(),
        normalizedPath.end(),
        '\\',
        '/');

    while (!normalizedPath.empty() &&
           normalizedPath.back() == '/') {
        normalizedPath.pop_back();
    }

    const std::size_t separator =
        normalizedPath.rfind('/');

    const std::string segment =
        separator == std::string::npos
            ? normalizedPath
            : normalizedPath.substr(separator + 1);

    int year = 0;
    int month = 0;
    int day = 0;
    int hour = 0;
    int minute = 0;

    if (std::sscanf(
            segment.c_str(),
            "%4d-%2d-%2d.%2d.%2d",
            &year,
            &month,
            &day,
            &hour,
            &minute) != 5) {
        return -1;
    }

    if (year < 1970 ||
        month < 1 ||
        month > 12 ||
        day < 1 ||
        day > 31 ||
        hour < 0 ||
        hour > 23 ||
        minute < 0 ||
        minute > 59) {
        return -1;
    }

    std::tm timeParts {};
    timeParts.tm_year = year - 1900;
    timeParts.tm_mon = month - 1;
    timeParts.tm_mday = day;
    timeParts.tm_hour = hour;
    timeParts.tm_min = minute;
    timeParts.tm_sec = 0;
    timeParts.tm_isdst = -1;

    const std::time_t timestamp =
        std::mktime(&timeParts);

    if (timestamp <= 0) {
        return -1;
    }

    return static_cast<long long>(timestamp);
}

std::string extractRecordingsArray(const std::string& json)
{
    std::size_t recordingsKey = json.find("\"recordings\"");
    if (recordingsKey != std::string::npos) {
        std::size_t arrayStart = json.find('[', recordingsKey);
        if (arrayStart == std::string::npos) {
            return "";
        }

        std::size_t arrayEnd = findMatching(json, arrayStart, '[', ']');
        if (arrayEnd == std::string::npos) {
            return "";
        }

        return json.substr(arrayStart + 1, arrayEnd - arrayStart - 1);
    }

    std::size_t arrayStart = json.find('[');
    if (arrayStart == std::string::npos) {
        return "";
    }

    std::size_t arrayEnd = findMatching(json, arrayStart, '[', ']');
    if (arrayEnd == std::string::npos) {
        return "";
    }

    return json.substr(arrayStart + 1, arrayEnd - arrayStart - 1);
}

VdrRecording mapObjectToRecording(const std::string& objectText)
{
    VdrRecording recording;

    int number = getIntField(objectText, "number", -1);
    std::string fileName = getStringField(objectText, "file_name");
    std::string relativePath = getStringField(objectText, "relative_file_name");

    recording.id = number >= 0 ? std::to_string(number) : "";
    recording.title = normalizeRecordingName(getStringField(objectText, "name"));
    recording.path = relativePath.empty() ? fileName : relativePath;
    recording.backendNativeId = fileName;
    long long startTime =
        getLongLongField(objectText, "event_start_time", -1);

    if (startTime <= 0) {
        startTime =
            deriveRecordingStartTimeFromPath(recording.path);
    }

    if (startTime <= 0) {
        startTime =
            deriveRecordingStartTimeFromPath(fileName);
    }

    recording.startTime =
        startTime > 0 ? std::to_string(startTime) : "-1";

    int durationSeconds =
        getIntField(objectText, "duration", -1);
    recording.recordingDurationKnown = durationSeconds > 0;

    if (durationSeconds <= 0) {
        durationSeconds =
            getIntField(objectText, "event_duration", -1);
    }

    recording.durationSeconds = durationSeconds;
    recording.sizeMb = getLongLongField(objectText, "filesize_mb", 0);
    recording.persons = parseAdditionalMediaActors(objectText);

    return recording;
}

struct RecordingCandidate
{
    VdrRecording recording;
    std::string inode;
    std::string eventTitle;
    std::size_t originalIndex = 0;
};

std::string recordingSelectionPath(
    const VdrRecording& recording)
{
    if (!recording.backendNativeId.empty()) {
        return recording.backendNativeId;
    }

    return recording.path;
}

std::vector<std::string> splitRecordingPath(
    std::string path)
{
    std::replace(path.begin(), path.end(), '\\', '/');

    std::vector<std::string> segments;
    std::string current;

    for (char c : path) {
        if (c == '/') {
            if (!current.empty()) {
                segments.push_back(current);
                current.clear();
            }
            continue;
        }

        current.push_back(c);
    }

    if (!current.empty()) {
        segments.push_back(current);
    }

    return segments;
}

std::string recordingDirectoryLeaf(
    const VdrRecording& recording)
{
    const std::vector<std::string> segments =
        splitRecordingPath(recordingSelectionPath(recording));

    if (segments.empty()) {
        return "";
    }

    return segments.back();
}

std::size_t recordingPathDepth(
    const VdrRecording& recording)
{
    return splitRecordingPath(
        recordingSelectionPath(recording)).size();
}

std::string recordingAliasIdentityKey(
    const VdrRecording& recording)
{
    const std::string leaf =
        recordingDirectoryLeaf(recording);

    if (leaf.empty()) {
        return "";
    }

    return leaf + "|" +
           recording.startTime + "|" +
           std::to_string(recording.durationSeconds) + "|" +
           std::to_string(recording.sizeMb);
}

bool isAliasFamily(
    const std::vector<RecordingCandidate>& candidates,
    const std::vector<std::size_t>& indices)
{
    bool hasEmptyInode = false;
    bool hasNonEmptyInode = false;
    bool hasDuplicateInode = false;
    std::set<std::string> seenInodes;

    for (const std::size_t index : indices) {
        const RecordingCandidate& candidate =
            candidates.at(index);

        if (candidate.inode.empty()) {
            hasEmptyInode = true;
        } else {
            hasNonEmptyInode = true;
            if (!seenInodes.insert(candidate.inode).second) {
                hasDuplicateInode = true;
            }
        }

    }

    // Treat actual provider inode evidence as alias information.
    // A folder name is never an alias marker: it may be a real recording
    // category on any distribution or mounted backend.
    return (hasEmptyInode && hasNonEmptyInode) ||
           hasDuplicateInode;
}

bool preferRecordingCandidate(
    const RecordingCandidate& candidate,
    const RecordingCandidate& current)
{
    if (candidate.inode.empty() != current.inode.empty()) {
        return !candidate.inode.empty();
    }

    const std::size_t candidateDepth =
        recordingPathDepth(candidate.recording);
    const std::size_t currentDepth =
        recordingPathDepth(current.recording);

    if (candidateDepth != currentDepth) {
        return candidateDepth < currentDepth;
    }

    const std::size_t candidateLength =
        recordingSelectionPath(candidate.recording).size();
    const std::size_t currentLength =
        recordingSelectionPath(current.recording).size();

    if (candidateLength != currentLength) {
        return candidateLength < currentLength;
    }

    return candidate.originalIndex > current.originalIndex;
}

std::vector<VdrRecording> canonicalizeRecordingAliases(
    const std::vector<RecordingCandidate>& candidates)
{
    std::map<std::string, std::vector<std::size_t>> groups;

    for (std::size_t index = 0; index < candidates.size(); ++index) {
        const std::string key =
            recordingAliasIdentityKey(candidates.at(index).recording);

        if (!key.empty()) {
            groups[key].push_back(index);
        }
    }

    std::vector<bool> keep(candidates.size(), true);

    for (const auto& entry : groups) {
        const std::vector<std::size_t>& indices =
            entry.second;

        if (indices.size() < 2 ||
            !isAliasFamily(candidates, indices)) {
            continue;
        }

        std::size_t preferredIndex =
            indices.front();

        for (const std::size_t index : indices) {
            if (preferRecordingCandidate(
                    candidates.at(index),
                    candidates.at(preferredIndex))) {
                preferredIndex = index;
            }
        }

        for (const std::size_t index : indices) {
            keep.at(index) = index == preferredIndex;
        }
    }

    std::vector<VdrRecording> recordings;

    for (std::size_t index = 0; index < candidates.size(); ++index) {
        if (keep.at(index)) {
            recordings.push_back(
                candidates.at(index).recording);
        }
    }

    return recordings;
}

}

// Keep native Stage paths and opaque identities unchanged. The VDR provider
// may expose a completed HEVC candidate under its private directory; the
// catalog shows a friendly title derived from the ORIGINAL recording instead.
static bool isPrivateMediaShrinkStage(const VdrRecording& recording)
{
    constexpr const char* marker = "/.vdr-suite-shrink-";
    return recording.path.find(marker) != std::string::npos ||
           recording.backendNativeId.find(marker) != std::string::npos;
}

static std::string recordingDirectoryName(const VdrRecording& recording)
{
    const std::string& path = recording.path.empty()
        ? recording.backendNativeId : recording.path;
    const auto slash = path.rfind('/');
    return slash == std::string::npos ? path : path.substr(slash + 1);
}

static std::string recordingTitleLeaf(const VdrRecording& recording)
{
    const auto slash = recording.title.rfind('/');
    const std::string value = slash == std::string::npos
        ? recording.title : recording.title.substr(slash + 1);
    return value;
}

// Same exact VDR .rec directory name, not just a broadcast time or metadata
// title. If multiple original copies exist with different names, don't guess.
static std::string sourceTitleForPrivateStage(
    const std::vector<RecordingCandidate>& candidates,
    const RecordingCandidate& stage)
{
    const std::string recLeaf = recordingDirectoryName(stage.recording);
    if (recLeaf.size() < 5 ||
        recLeaf.compare(recLeaf.size() - 4, 4, ".rec") != 0)
        return {};
    std::string title;
    for (const auto& possible : candidates) {
        if (isPrivateMediaShrinkStage(possible.recording) ||
            recordingDirectoryName(possible.recording) != recLeaf)
            continue;
        const std::string leaf = recordingTitleLeaf(possible.recording);
        if (leaf.empty()) continue;
        // An already published HEVC sibling can share the same .rec leaf.
        // It is not another source title and must not make this match
        // ambiguous or create a doubled ".shrink.shrink" display suffix.
        if ((leaf.size() >= 7 &&
             leaf.compare(leaf.size() - 7, 7, ".shrink") == 0) ||
            (leaf.size() >= 5 &&
             leaf.compare(leaf.size() - 5, 5, "_HEVC") == 0))
            continue;
        if (!title.empty() && title != leaf) return {};
        title = leaf;
    }
    return title;
}

std::vector<VdrRecording> RestfulApiRecordingMapper::parseRecordings(const std::string& json)
{
    std::vector<RecordingCandidate> candidates;

    std::string arrayText = extractRecordingsArray(json);
    if (arrayText.empty()) {
        return {};
    }

    std::vector<std::string> objects = splitTopLevelObjects(arrayText);
    for (const std::string& objectText : objects) {
        RecordingCandidate candidate;
        candidate.recording = mapObjectToRecording(objectText);
        candidate.inode = getStringField(objectText, "inode");
        candidate.eventTitle = getStringField(objectText, "event_title");
        candidate.originalIndex = candidates.size();

        if (!candidate.recording.id.empty()) {
            candidates.push_back(candidate);
        }
    }

    // Keep the converted recording visible in VDR-Suite without moving the
    // private candidate or breaking the durable Stage/Review binding.
    for (auto& candidate : candidates) {
        if (!isPrivateMediaShrinkStage(candidate.recording)) continue;
        std::string sourceTitle =
            sourceTitleForPrivateStage(candidates, candidate);
        if (sourceTitle.empty()) {
            // The original may be unavailable in this inventory. Prefer the
            // VDR event title to leaking the temporary working-directory ID.
            sourceTitle = candidate.eventTitle;
        }
        if (sourceTitle.empty()) sourceTitle = "HEVC-Aufnahme";
        candidate.recording.title = sourceTitle + ".shrink";
    }

    return canonicalizeRecordingAliases(candidates);
}