#include "recording_name.h"

#include <algorithm>
#include <cctype>
#include <vector>

namespace vdrsuite::agent {
namespace {

bool hasControlCharacter(const std::string& value)
{
    return std::any_of(
        value.begin(),
        value.end(),
        [](unsigned char c) { return std::iscntrl(c) != 0; });
}

std::vector<std::string> splitFolder(const std::string& value)
{
    std::vector<std::string> parts;
    std::string current;

    for (char c : value) {
        if (c == '/' || c == '~') {
            if (!current.empty()) {
                parts.push_back(current);
                current.clear();
            }
            continue;
        }

        current.push_back(c);
    }

    if (!current.empty()) {
        parts.push_back(current);
    }

    return parts;
}

std::string recordingLeaf(const std::string& currentRecordingName)
{
    const std::size_t separator = currentRecordingName.find_last_of('~');
    if (separator == std::string::npos) {
        return currentRecordingName;
    }

    return currentRecordingName.substr(separator + 1);
}

} // namespace

RecordingMoveNameResult buildMovedRecordingName(
    const std::string& currentRecordingName,
    const std::string& targetFolder)
{
    RecordingMoveNameResult result;

    if (currentRecordingName.empty()) {
        result.error = "current recording name is empty";
        return result;
    }

    if (hasControlCharacter(currentRecordingName) ||
        hasControlCharacter(targetFolder)) {
        result.error = "recording name or target folder contains control characters";
        return result;
    }

    const std::string leaf = recordingLeaf(currentRecordingName);
    if (leaf.empty() || leaf == "." || leaf == "..") {
        result.error = "recording leaf name is invalid";
        return result;
    }

    std::string normalizedFolder;
    if (!targetFolder.empty() && targetFolder != "/") {
        const std::vector<std::string> parts = splitFolder(targetFolder);

        for (const std::string& part : parts) {
            if (part == "." || part == "..") {
                result.error = "target folder contains an unsafe path component";
                return result;
            }

            if (!normalizedFolder.empty()) {
                normalizedFolder += '~';
            }
            normalizedFolder += part;
        }
    }

    result.newName = normalizedFolder.empty()
        ? leaf
        : normalizedFolder + "~" + leaf;
    result.success = true;
    return result;
}

} // namespace vdrsuite::agent
