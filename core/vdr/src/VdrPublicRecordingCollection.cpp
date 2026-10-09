#include "VdrPublicRecordingCollection.h"

#include <algorithm>
#include <map>
#include <set>
#include <sstream>
#include <string>

namespace
{
std::string scopedKey(const std::string& backend, const std::string& address)
{
    std::string result = backend;
    result.push_back('\0');
    result += address;
    return result;
}

bool publicIdValid(const std::string& id)
{
    if (id.size() != 36 || id.compare(0, 4, "rec_") != 0)
        return false;
    for (std::size_t i = 4; i < id.size(); ++i)
        if (!((id[i] >= '0' && id[i] <= '9') ||
              (id[i] >= 'a' && id[i] <= 'f')))
            return false;
    return true;
}

std::string jsonString(const std::string& value)
{
    const char* hex = "0123456789abcdef";
    std::string output = "\"";
    for (unsigned char character : value)
    {
        if (character == '"' || character == '\\')
        {
            output.push_back('\\');
            output.push_back(static_cast<char>(character));
        }
        else if (character < 0x20)
        {
            output += "\\u00";
            output.push_back(hex[character >> 4]);
            output.push_back(hex[character & 0x0f]);
        }
        else output.push_back(static_cast<char>(character));
    }
    output += "\"";
    return output;
}
}

VdrPublicRecordingCollection projectPublicRecordings(
    const std::vector<VdrRecording>& cachedRecordings,
    const std::vector<VdrPublicRecordingBinding>& activeBindings,
    const std::vector<std::string>& authorizedBackendIds)
{
    VdrPublicRecordingCollection result;
    const std::set<std::string> allowed(authorizedBackendIds.begin(),
                                         authorizedBackendIds.end());
    if (allowed.empty()) return result;

    std::map<std::string, std::string> idsBySource;
    std::set<std::string> uniqueIds;
    for (const auto& binding : activeBindings)
    {
        if (allowed.count(binding.backendId) == 0) continue;
        if (binding.sourceAddress.empty() ||
            !publicIdValid(binding.publicRecordingId) ||
            !uniqueIds.insert(binding.publicRecordingId).second ||
            !idsBySource.emplace(scopedKey(binding.backendId, binding.sourceAddress),
                                 binding.publicRecordingId).second)
        {
            result.valid = false;
            return result;
        }
    }

    std::set<std::string> seenSources;
    for (const auto& recording : cachedRecordings)
    {
        if (allowed.count(recording.backendId) == 0) continue;
        const std::string address = recording.backendNativeId.empty()
            ? recording.path : recording.backendNativeId;
        const std::string source = scopedKey(recording.backendId, address);
        if (address.empty() || recording.startTime.empty() ||
            !seenSources.insert(source).second)
        {
            result.valid = false;
            result.items.clear();
            return result;
        }
        const auto identity = idsBySource.find(source);
        if (identity == idsBySource.end())
        {
            // An incomplete/reconciling identity index must be reported
            // unavailable, not mistaken for an empty Recording library.
            result.valid = false;
            result.items.clear();
            return result;
        }
        VdrPublicRecordingSummary summary;
        summary.recordingId = identity->second;
        summary.backendId = recording.backendId;
        summary.title = recording.title.empty() ? "Unbenannte Aufnahme" :
                        recording.title;
        summary.recordedAt = recording.startTime;
        summary.durationKnown = recording.recordingDurationKnown &&
                                 recording.durationSeconds > 0;
        summary.durationSeconds = summary.durationKnown ?
                                  recording.durationSeconds : 0;
        result.items.push_back(std::move(summary));
    }

    std::sort(result.items.begin(), result.items.end(),
        [](const auto& left, const auto& right) {
            if (left.recordedAt != right.recordedAt)
                return left.recordedAt > right.recordedAt;
            return left.recordingId < right.recordingId;
        });
    return result;
}

std::string serializePublicRecordingCollection(
    const VdrPublicRecordingCollection& collection)
{
    if (!collection.valid) return {};
    std::ostringstream stream;
    stream << "{\"items\":[";
    for (std::size_t index = 0; index < collection.items.size(); ++index)
    {
        const auto& item = collection.items[index];
        if (index != 0) stream << ",";
        stream << "{\"recordingId\":" << jsonString(item.recordingId)
               << ",\"backendId\":" << jsonString(item.backendId)
               << ",\"title\":" << jsonString(item.title)
               << ",\"recordedAt\":" << jsonString(item.recordedAt)
               << ",\"durationKnown\":" << (item.durationKnown ? "true" : "false")
               << ",\"durationSeconds\":" << item.durationSeconds
               << "}";
    }
    stream << "],\"meta\":{\"partial\":false}}";
    return stream.str();
}
