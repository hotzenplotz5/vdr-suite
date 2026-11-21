#include "Database.h"
#include "EpgSeriesArtworkFallbackOrphanCleaner.h"
#include "EpgSeriesArtworkFallbackRepository.h"

#include <cassert>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

namespace
{
std::string hexComponent(const std::string& value)
{
    static constexpr char Digits[] = "0123456789abcdef";
    std::string encoded;
    encoded.reserve(value.size() * 2U);
    for (const unsigned char byte : value)
    {
        encoded.push_back(Digits[(byte >> 4U) & 0x0fU]);
        encoded.push_back(Digits[byte & 0x0fU]);
    }
    return encoded;
}

std::filesystem::path managedPath(
    const std::filesystem::path& root,
    const std::string& backendId,
    const std::string& channelId,
    const std::string& eventId,
    const std::string& filename = "series.png")
{
    return root /
        hexComponent(backendId) /
        hexComponent(channelId) /
        hexComponent(eventId) /
        filename;
}

void writeFile(
    const std::filesystem::path& path,
    const std::string& content = "managed artwork")
{
    std::filesystem::create_directories(path.parent_path());
    std::ofstream file(path, std::ios::binary);
    assert(file);
    file << content;
    assert(file.good());
}

void setModifiedAt(
    const std::filesystem::path& path,
    std::int64_t timestamp)
{
    const timespec times[2] = {
        {static_cast<time_t>(timestamp), 0},
        {static_cast<time_t>(timestamp), 0}
    };
    assert(::utimensat(
        AT_FDCWD,
        path.c_str(),
        times,
        AT_SYMLINK_NOFOLLOW) == 0);
}

EpgArtworkReference referenceFor(
    const std::string& eventId,
    const std::filesystem::path& path)
{
    EpgArtworkReference reference;
    reference.backendId = "backend";
    reference.channelId = "channel";
    reference.eventId = eventId;
    reference.provider = "tmdb";
    reference.origin = EpgArtworkReferenceOrigin::ExternalFallback;
    reference.path = path.string();
    reference.width = 1280;
    reference.height = 720;
    reference.resolvedAt = 1234;
    return reference;
}

EpgSeriesArtworkFallbackOrphanCleanupConfig cleanupConfig(
    const std::filesystem::path& root,
    std::size_t maximumFiles = 64U)
{
    EpgSeriesArtworkFallbackOrphanCleanupConfig config;
    config.enabled = true;
    config.cacheRoot = root.string();
    config.minimumAgeSeconds = 60 * 60;
    config.maximumFilesPerRun = maximumFiles;
    return config;
}
}

int main()
{
    const std::int64_t now = 2000000;
    const std::int64_t old = now - 2 * 60 * 60;
    const std::filesystem::path root =
        std::filesystem::temp_directory_path() /
        ("vdr-suite-fallback-orphan-cleaner-" +
         std::to_string(::getpid()));
    std::filesystem::remove_all(root);

    Database database;
    assert(database.open(":memory:"));
    EpgSeriesArtworkFallbackRepository repository(database);
    assert(repository.ensureSchema());

    const std::filesystem::path referenced = managedPath(
        root,
        "backend",
        "channel",
        "referenced");
    writeFile(referenced);
    setModifiedAt(referenced, old);
    assert(repository.upsert(referenceFor("referenced", referenced)));

    const std::filesystem::path orphan = managedPath(
        root,
        "backend",
        "channel",
        "orphan");
    writeFile(orphan);
    setModifiedAt(orphan, old);

    const std::filesystem::path young = managedPath(
        root,
        "backend",
        "channel",
        "young");
    writeFile(young);
    setModifiedAt(young, now - 60);

    const std::filesystem::path foreign = managedPath(
        root,
        "backend",
        "channel",
        "foreign",
        "cover.png");
    writeFile(foreign);
    setModifiedAt(foreign, old);

    const std::filesystem::path temporary = managedPath(
        root,
        "backend",
        "channel",
        "temporary",
        ".series.crash.tmp");
    writeFile(temporary);
    setModifiedAt(temporary, old);

    const std::filesystem::path outside = root.parent_path() /
        ("vdr-suite-fallback-orphan-outside-" +
         std::to_string(::getpid()) + ".png");
    writeFile(outside, "outside");
    setModifiedAt(outside, old);

    const std::filesystem::path symlink = managedPath(
        root,
        "backend",
        "channel",
        "symlink");
    std::filesystem::create_directories(symlink.parent_path());
    std::filesystem::create_symlink(outside, symlink);

    const std::filesystem::path outsideDirectory = root.parent_path() /
        ("vdr-suite-fallback-orphan-directory-" +
         std::to_string(::getpid()));
    const std::filesystem::path outsideNested =
        outsideDirectory / "series.png";
    writeFile(outsideNested, "outside directory");
    setModifiedAt(outsideNested, old);
    const std::filesystem::path linkedEventDirectory =
        managedPath(
            root,
            "backend",
            "channel",
            "linked-event").parent_path();
    std::filesystem::create_directories(
        linkedEventDirectory.parent_path());
    std::filesystem::create_directory_symlink(
        outsideDirectory,
        linkedEventDirectory);

    const std::filesystem::path invalidTree =
        root / "not-hex" / "channel" / "event" / "series.png";
    writeFile(invalidTree);
    setModifiedAt(invalidTree, old);

    EpgSeriesArtworkFallbackOrphanCleaner cleaner(
        repository,
        cleanupConfig(root));
    const EpgSeriesArtworkFallbackOrphanCleanupResult result =
        cleaner.cleanup(now);
    assert(result.attempted);
    assert(result.rootAvailable);
    assert(result.succeeded());
    assert(!result.limitReached);
    assert(result.removedFiles == 1U);
    assert(result.referencedFiles == 1U);
    assert(result.youngFiles == 1U);
    assert(result.skippedUnsafeEntries >= 2U);
    assert(std::filesystem::exists(referenced));
    assert(!std::filesystem::exists(orphan));
    assert(std::filesystem::exists(young));
    assert(std::filesystem::exists(foreign));
    assert(std::filesystem::exists(temporary));
    assert(std::filesystem::is_symlink(symlink));
    assert(std::filesystem::exists(outside));
    assert(std::filesystem::is_symlink(linkedEventDirectory));
    assert(std::filesystem::exists(outsideNested));
    assert(std::filesystem::exists(invalidTree));
    assert(!std::filesystem::exists(orphan.parent_path()));

    const std::filesystem::path batchRoot = root.parent_path() /
        ("vdr-suite-fallback-orphan-batch-" +
         std::to_string(::getpid()));
    const std::filesystem::path batchOne = managedPath(
        batchRoot,
        "backend",
        "channel",
        "batch-one");
    const std::filesystem::path batchTwo = managedPath(
        batchRoot,
        "backend",
        "channel",
        "batch-two");
    writeFile(batchOne);
    writeFile(batchTwo);
    setModifiedAt(batchOne, old);
    setModifiedAt(batchTwo, old);

    EpgSeriesArtworkFallbackOrphanCleaner batchCleaner(
        repository,
        cleanupConfig(batchRoot, 1U));
    const auto firstBatch = batchCleaner.cleanup(now);
    assert(firstBatch.succeeded());
    assert(firstBatch.limitReached);
    assert(firstBatch.removedFiles == 1U);
    assert(std::filesystem::exists(batchOne) !=
           std::filesystem::exists(batchTwo));

    const auto secondBatch = batchCleaner.cleanup(now);
    assert(secondBatch.succeeded());
    assert(secondBatch.removedFiles == 1U);
    assert(!std::filesystem::exists(batchOne));
    assert(!std::filesystem::exists(batchTwo));

    const std::filesystem::path disabledRoot = root.parent_path() /
        ("vdr-suite-fallback-orphan-disabled-" +
         std::to_string(::getpid()));
    const std::filesystem::path disabledOrphan = managedPath(
        disabledRoot,
        "backend",
        "channel",
        "disabled");
    writeFile(disabledOrphan);
    setModifiedAt(disabledOrphan, old);
    auto disabledConfig = cleanupConfig(disabledRoot);
    disabledConfig.enabled = false;
    EpgSeriesArtworkFallbackOrphanCleaner disabledCleaner(
        repository,
        disabledConfig);
    const auto disabledResult = disabledCleaner.cleanup(now);
    assert(!disabledResult.attempted);
    assert(disabledResult.succeeded());
    assert(std::filesystem::exists(disabledOrphan));

    const std::filesystem::path symlinkRootTarget = root.parent_path() /
        ("vdr-suite-fallback-orphan-root-target-" +
         std::to_string(::getpid()));
    const std::filesystem::path rootTargetOrphan = managedPath(
        symlinkRootTarget,
        "backend",
        "channel",
        "root-target");
    writeFile(rootTargetOrphan);
    setModifiedAt(rootTargetOrphan, old);
    const std::filesystem::path symlinkRoot = root.parent_path() /
        ("vdr-suite-fallback-orphan-root-link-" +
         std::to_string(::getpid()));
    std::filesystem::create_directory_symlink(
        symlinkRootTarget,
        symlinkRoot);
    EpgSeriesArtworkFallbackOrphanCleaner symlinkRootCleaner(
        repository,
        cleanupConfig(symlinkRoot));
    const auto symlinkRootResult = symlinkRootCleaner.cleanup(now);
    assert(symlinkRootResult.attempted);
    assert(!symlinkRootResult.succeeded());
    assert(!symlinkRootResult.rootAvailable);
    assert(std::filesystem::exists(rootTargetOrphan));

    const std::filesystem::path missingRoot = root.parent_path() /
        ("vdr-suite-fallback-orphan-missing-" +
         std::to_string(::getpid()));
    EpgSeriesArtworkFallbackOrphanCleaner missingRootCleaner(
        repository,
        cleanupConfig(missingRoot));
    const auto missingRootResult = missingRootCleaner.cleanup(now);
    assert(missingRootResult.attempted);
    assert(missingRootResult.succeeded());
    assert(!missingRootResult.rootAvailable);

    std::filesystem::remove_all(root);
    std::filesystem::remove_all(batchRoot);
    std::filesystem::remove_all(disabledRoot);
    std::filesystem::remove_all(symlinkRootTarget);
    std::filesystem::remove(symlinkRoot);
    std::filesystem::remove(outside);
    std::filesystem::remove_all(outsideDirectory);
    return 0;
}
