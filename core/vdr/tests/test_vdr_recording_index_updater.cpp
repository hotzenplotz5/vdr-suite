#include "VdrRecordingIndexUpdater.h"

#include <atomic>
#include <cassert>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>
#include <sys/stat.h>
#include <thread>
#include <unistd.h>
#include <vector>

namespace
{

bool waitForState(
    VdrRecordingIndexUpdater& updater,
    const std::string& directory,
    VdrRecordingIndexUpdateState expected)
{
    for (int attempt = 0; attempt < 100; ++attempt) {
        if (updater.status(directory).state == expected) return true;
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    return false;
}

} // namespace

int main()
{
    const std::filesystem::path root =
        std::filesystem::temp_directory_path() /
        ("vdr-suite-index-updater-test-" + std::to_string(::getpid()) + ".rec");
    std::filesystem::remove_all(root);
    std::filesystem::create_directories(root);
    const std::filesystem::path segment = root / "00001.ts";
    {
        std::ofstream output(segment, std::ios::binary);
        output << "transport-stream";
    }

    std::atomic<bool> release{false};
    int spawnCalls = 0;
    std::string spawnedDirectory;
    uid_t spawnedUid = static_cast<uid_t>(-1);
    gid_t spawnedGid = static_cast<gid_t>(-1);
    VdrRecordingIndexUpdater updater(
        [&](const std::string& directory, uid_t uid, gid_t gid) {
            ++spawnCalls;
            spawnedDirectory = directory;
            spawnedUid = uid;
            spawnedGid = gid;
            return static_cast<pid_t>(4242);
        },
        [&](pid_t pid) {
            assert(pid == 4242);
            while (!release.load())
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            return 0;
        });

    const auto started = updater.ensure(root.string(), {segment.string()});
    assert(started.running());
    assert(spawnCalls == 1);
    assert(std::filesystem::path(spawnedDirectory) ==
        std::filesystem::weakly_canonical(root));
    struct stat directoryStatus {};
    assert(::stat(root.c_str(), &directoryStatus) == 0);
    assert(spawnedUid == directoryStatus.st_uid);
    assert(spawnedGid == directoryStatus.st_gid);

    const auto duplicate = updater.ensure(root.string(), {segment.string()});
    assert(duplicate.running());
    assert(spawnCalls == 1);

    release.store(true);
    assert(waitForState(
        updater,
        root.string(),
        VdrRecordingIndexUpdateState::Succeeded));
    assert(updater.ensure(root.string(), {segment.string()}).succeeded());
    assert(spawnCalls == 1);

    const std::filesystem::path activeRoot = root.parent_path() /
        ("vdr-suite-index-updater-active-" + std::to_string(::getpid()) + ".rec");
    std::filesystem::create_directories(activeRoot);
    const std::filesystem::path activeSegment = activeRoot / "00001.ts";
    {
        std::ofstream output(activeSegment, std::ios::binary);
        output << "transport-stream";
        std::ofstream timer(activeRoot / ".timer");
        timer << "7@backend\n";
    }
    const auto active = updater.ensure(
        activeRoot.string(),
        {activeSegment.string()});
    assert(active.state == VdrRecordingIndexUpdateState::Failed);
    assert(active.reasonCode == "recording_index_update_recording_active");
    assert(spawnCalls == 1);

    // VDR 2.7.9 --updindex is an alias for --genindex, which unlinks
    // an existing index. Ensure the Suite refuses any index entry.
    const std::filesystem::path existingRoot = root.parent_path() /
        ("vdr-suite-index-updater-existing-" + std::to_string(::getpid()) + ".rec");
    std::filesystem::create_directories(existingRoot);
    const std::filesystem::path existingSegment = existingRoot / "00001.ts";
    const std::filesystem::path existingIndex = existingRoot / "index";
    {
        std::ofstream output(existingSegment, std::ios::binary);
        output << "transport-stream";
        std::ofstream idx(existingIndex, std::ios::binary);
        idx << "existing-original-index-must-survive";
    }
    const auto existing = updater.ensure(existingRoot.string(),
        {existingSegment.string()});
    assert(existing.state == VdrRecordingIndexUpdateState::Failed);
    assert(existing.reasonCode == "recording_index_update_existing_index_unsafe");
    {
        std::ifstream indexStream(existingIndex, std::ios::binary);
        std::string indexContents;
        std::getline(indexStream, indexContents);
        assert(indexContents == "existing-original-index-must-survive");
    }
    assert(spawnCalls == 1);
    std::filesystem::remove(existingIndex);

    // A dangling symlink must not be interpreted as a missing index.
    std::filesystem::create_symlink("missing-target", existingIndex);
    const auto symlinked = updater.ensure(existingRoot.string(),
        {existingSegment.string()});
    assert(symlinked.state == VdrRecordingIndexUpdateState::Failed);
    assert(symlinked.reasonCode == "recording_index_update_existing_index_unsafe");
    assert(std::filesystem::is_symlink(existingIndex));
    assert(spawnCalls == 1);
    std::filesystem::remove(existingIndex);
    {
        std::ofstream legacyIndex(existingRoot / "index.vdr", std::ios::binary);
        legacyIndex << "preserve legacy index entry";
    }
    const auto legacyIndexResult = updater.ensure(existingRoot.string(),
        {existingSegment.string()});
    assert(legacyIndexResult.state == VdrRecordingIndexUpdateState::Failed);
    assert(legacyIndexResult.reasonCode ==
        "recording_index_update_existing_index_unsafe");
    assert(spawnCalls == 1);

    const std::filesystem::path legacyRoot = root.parent_path() /
        ("vdr-suite-index-updater-legacy-" + std::to_string(::getpid()) + ".rec");
    std::filesystem::create_directories(legacyRoot);
    const std::filesystem::path legacySegment = legacyRoot / "001.vdr";
    {
        std::ofstream output(legacySegment, std::ios::binary);
        output << "legacy";
    }
    const auto legacy = updater.ensure(
        legacyRoot.string(),
        {legacySegment.string()});
    assert(legacy.state == VdrRecordingIndexUpdateState::Failed);
    assert(legacy.reasonCode == "recording_index_update_not_ts");
    assert(spawnCalls == 1);

    int failedSpawnCalls = 0;
    VdrRecordingIndexUpdater failedUpdater(
        [&](const std::string&, uid_t, gid_t) {
            ++failedSpawnCalls;
            return static_cast<pid_t>(-1);
        });
    const auto failed = failedUpdater.ensure(root.string(), {segment.string()});
    assert(failed.state == VdrRecordingIndexUpdateState::Failed);
    assert(failed.reasonCode == "recording_index_update_start_failed");
    assert(failedSpawnCalls == 1);

    std::filesystem::remove_all(root);
    std::filesystem::remove_all(activeRoot);
    std::filesystem::remove_all(legacyRoot);
    std::filesystem::remove_all(existingRoot);
    return 0;
}
