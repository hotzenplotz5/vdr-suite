#!/usr/bin/env python3
from pathlib import Path

ROOT = Path("/home/yavdr/vdr-suite")


def replace_once(relative_path: str, old: str, new: str) -> None:
    path = ROOT / relative_path
    text = path.read_text(encoding="utf-8")
    count = text.count(old)
    if count != 1:
        raise SystemExit(
            f"{relative_path}: expected exactly one replacement match, found {count}"
        )
    path.write_text(text.replace(old, new, 1), encoding="utf-8")
    print(f"updated {relative_path}")


replace_once(
    "core/vdr/src/RestfulApiRecordingMapper.cpp",
    """#include <ctime>
#include <string>
#include <vector>
""",
    """#include <ctime>
#include <map>
#include <set>
#include <string>
#include <vector>
"""
)

replace_once(
    "core/vdr/src/RestfulApiRecordingMapper.cpp",
    """    return recording;
}

}

std::vector<VdrRecording> RestfulApiRecordingMapper::parseRecordings(const std::string& json)
{
    std::vector<VdrRecording> recordings;

    std::string arrayText = extractRecordingsArray(json);
    if (arrayText.empty()) {
        return recordings;
    }

    std::vector<std::string> objects = splitTopLevelObjects(arrayText);
    for (const std::string& objectText : objects) {
        VdrRecording recording = mapObjectToRecording(objectText);
        if (!recording.id.empty()) {
            recordings.push_back(recording);
        }
    }

    return recordings;
}
""",
    """    return recording;
}

struct RecordingCandidate
{
    VdrRecording recording;
    std::string inode;
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
    std::replace(path.begin(), path.end(), '\\\\', '/');

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

std::size_t storageMountSegmentCount(
    const VdrRecording& recording)
{
    const std::vector<std::string> segments =
        splitRecordingPath(recordingSelectionPath(recording));

    return static_cast<std::size_t>(std::count(
        segments.begin(),
        segments.end(),
        "Recordings_on_yavdr(nfs)"));
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
    bool hasRepeatedStorageMount = false;
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

        if (storageMountSegmentCount(candidate.recording) > 1) {
            hasRepeatedStorageMount = true;
        }
    }

    return (hasEmptyInode && hasNonEmptyInode) ||
           hasRepeatedStorageMount ||
           hasDuplicateInode;
}

bool preferRecordingCandidate(
    const RecordingCandidate& candidate,
    const RecordingCandidate& current)
{
    if (candidate.inode.empty() != current.inode.empty()) {
        return !candidate.inode.empty();
    }

    const std::size_t candidateMountCount =
        storageMountSegmentCount(candidate.recording);
    const std::size_t currentMountCount =
        storageMountSegmentCount(current.recording);

    if (candidateMountCount != currentMountCount) {
        return candidateMountCount < currentMountCount;
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
        candidate.originalIndex = candidates.size();

        if (!candidate.recording.id.empty()) {
            candidates.push_back(candidate);
        }
    }

    return canonicalizeRecordingAliases(candidates);
}
"""
)

replace_once(
    "core/vdr/tests/test_restful_api_recording_mapper.cpp",
    """static void test_parse_recordings_tolerates_invalid_json()
{
""",
    """static void test_parse_recordings_prefers_canonical_entry_over_recursive_aliases()
{
    const std::string json =
        "{\\\"recordings\\\":["
        "{\\\"number\\\":3652,"
        "\\\"name\\\":\\\"Recordings on yavdr(nfs)~Recordings on yavdr(nfs)~heute journal\\\","
        "\\\"file_name\\\":\\\"/srv/vdr/video/Recordings_on_yavdr(nfs)/Recordings_on_yavdr(nfs)/heute_journal/2026-07-08.21.45.2-0.rec\\\","
        "\\\"relative_file_name\\\":\\\"/Recordings_on_yavdr(nfs)/Recordings_on_yavdr(nfs)/heute_journal/2026-07-08.21.45.2-0.rec\\\","
        "\\\"inode\\\":\\\"\\\","
        "\\\"duration\\\":341,"
        "\\\"filesize_mb\\\":284,"
        "\\\"event_start_time\\\":1783539900},"
        "{\\\"number\\\":7378,"
        "\\\"name\\\":\\\"Recordings on yavdr(nfs)~heute journal\\\","
        "\\\"file_name\\\":\\\"/srv/vdr/video/Recordings_on_yavdr(nfs)/heute_journal/2026-07-08.21.45.2-0.rec\\\","
        "\\\"relative_file_name\\\":\\\"/Recordings_on_yavdr(nfs)/heute_journal/2026-07-08.21.45.2-0.rec\\\","
        "\\\"inode\\\":\\\"\\\","
        "\\\"duration\\\":341,"
        "\\\"filesize_mb\\\":284,"
        "\\\"event_start_time\\\":1783539900},"
        "{\\\"number\\\":7999,"
        "\\\"name\\\":\\\"VDR-SUITE-TEST heute journal\\\","
        "\\\"file_name\\\":\\\"/srv/vdr/video/VDR-SUITE-TEST_heute_journal/2026-07-08.21.45.2-0.rec\\\","
        "\\\"relative_file_name\\\":\\\"/VDR-SUITE-TEST_heute_journal/2026-07-08.21.45.2-0.rec\\\","
        "\\\"inode\\\":\\\"2065:14947898\\\","
        "\\\"duration\\\":341,"
        "\\\"filesize_mb\\\":284,"
        "\\\"event_start_time\\\":1783539900}"
        "]}";

    const std::vector<VdrRecording> recordings =
        RestfulApiRecordingMapper::parseRecordings(json);

    assert(recordings.size() == 1);
    assert(recordings[0].id == "7999");
    assert(recordings[0].title == "VDR-SUITE-TEST heute journal");
    assert(recordings[0].backendNativeId ==
        "/srv/vdr/video/VDR-SUITE-TEST_heute_journal/2026-07-08.21.45.2-0.rec");
}

static void test_parse_recordings_preserves_distinct_real_copies()
{
    const std::string json =
        "{\\\"recordings\\\":["
        "{\\\"number\\\":10,"
        "\\\"name\\\":\\\"Copy A\\\","
        "\\\"file_name\\\":\\\"/srv/vdr/video/Copy_A/2026-07-08.21.45.2-0.rec\\\","
        "\\\"inode\\\":\\\"2065:100\\\","
        "\\\"duration\\\":341,"
        "\\\"filesize_mb\\\":284,"
        "\\\"event_start_time\\\":1783539900},"
        "{\\\"number\\\":11,"
        "\\\"name\\\":\\\"Copy B\\\","
        "\\\"file_name\\\":\\\"/srv/vdr/video/Copy_B/2026-07-08.21.45.2-0.rec\\\","
        "\\\"inode\\\":\\\"2065:101\\\","
        "\\\"duration\\\":341,"
        "\\\"filesize_mb\\\":284,"
        "\\\"event_start_time\\\":1783539900}"
        "]}";

    const std::vector<VdrRecording> recordings =
        RestfulApiRecordingMapper::parseRecordings(json);

    assert(recordings.size() == 2);
}

static void test_parse_recordings_tolerates_invalid_json()
{
"""
)

replace_once(
    "core/vdr/tests/test_restful_api_recording_mapper.cpp",
    """    test_parse_recordings_derives_start_time_from_rec_directory();
    test_parse_recordings_tolerates_invalid_json();
""",
    """    test_parse_recordings_derives_start_time_from_rec_directory();
    test_parse_recordings_prefers_canonical_entry_over_recursive_aliases();
    test_parse_recordings_preserves_distinct_real_copies();
    test_parse_recordings_tolerates_invalid_json();
"""
)

print("recording alias canonicalization production fix applied")
