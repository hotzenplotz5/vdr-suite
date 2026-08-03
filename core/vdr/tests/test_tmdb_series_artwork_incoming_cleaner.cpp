#include "TmdbSeriesArtworkIncomingCleaner.h"

#include <cassert>
#include <cstdlib>
#include <fcntl.h>
#include <filesystem>
#include <fstream>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

namespace
{
std::filesystem::path temporaryDirectory(const std::string& label)
{
    std::string pattern = "/tmp/vdr-suite-" + label + "-XXXXXX";
    char* created = ::mkdtemp(pattern.data());
    assert(created != nullptr);
    return std::filesystem::path(created);
}

void writeFile(const std::filesystem::path& path)
{
    std::ofstream output(path, std::ios::binary);
    assert(output.good());
    output << "candidate";
    output.close();
    assert(output.good());
}

void setModificationTime(
    const std::filesystem::path& path,
    long long epochSeconds)
{
    struct timespec times[2] {};
    times[0].tv_sec = epochSeconds;
    times[1].tv_sec = epochSeconds;
    assert(::utimensat(
        AT_FDCWD,
        path.c_str(),
        times,
        AT_SYMLINK_NOFOLLOW) == 0);
}

TmdbSeriesArtworkIncomingCleanupConfig enabledConfig(
    const std::filesystem::path& root,
    std::size_t maximumFiles = 64U)
{
    TmdbSeriesArtworkIncomingCleanupConfig config;
    config.enabled = true;
    config.incomingRoot = root.string();
    config.minimumAgeSeconds = 100;
    config.maximumFilesPerRun = maximumFiles;
    return config;
}
}

int main()
{
    constexpr long long Now = 2000000;
    constexpr long long Old = Now - 1000;
    constexpr long long Young = Now - 10;

    {
        TmdbSeriesArtworkIncomingCleanupConfig config;
        config.incomingRoot = "/tmp/not-used";
        TmdbSeriesArtworkIncomingCleaner cleaner(config);
        const auto result = cleaner.cleanup(Now);
        assert(!result.attempted);
        assert(result.succeeded());
        assert(result.removedFiles() == 0U);
    }

    {
        const std::filesystem::path parent = temporaryDirectory("incoming-missing");
        const std::filesystem::path missing = parent / "missing";
        TmdbSeriesArtworkIncomingCleaner cleaner(enabledConfig(missing));
        const auto result = cleaner.cleanup(Now);
        assert(result.attempted);
        assert(!result.rootAvailable);
        assert(result.succeeded());
        assert(result.removedFiles() == 0U);
        std::filesystem::remove_all(parent);
    }

    {
        const std::filesystem::path root = temporaryDirectory("incoming-cleaner");
        const std::filesystem::path oldCandidate =
            root / "tmdb-123-0123456789abcdef.candidate";
        const std::filesystem::path oldTemporary =
            root / ".tmdb-456-fedcba9876543210.candidate.123.0.tmp";
        const std::filesystem::path youngCandidate =
            root / "tmdb-789-aaaaaaaaaaaaaaaa.candidate";
        const std::filesystem::path uppercaseHash =
            root / "tmdb-10-ABCDEF0123456789.candidate";
        const std::filesystem::path leadingZero =
            root / "tmdb-01-1111111111111111.candidate";
        const std::filesystem::path foreignTemporary =
            root / ".download.123.tmp";
        const std::filesystem::path external = root.parent_path() /
            (root.filename().string() + "-external");
        const std::filesystem::path candidateSymlink =
            root / "tmdb-321-bbbbbbbbbbbbbbbb.candidate";
        const std::filesystem::path candidateDirectory =
            root / "tmdb-654-cccccccccccccccc.candidate";

        writeFile(oldCandidate);
        writeFile(oldTemporary);
        writeFile(youngCandidate);
        writeFile(uppercaseHash);
        writeFile(leadingZero);
        writeFile(foreignTemporary);
        writeFile(external);
        std::filesystem::create_symlink(external, candidateSymlink);
        std::filesystem::create_directory(candidateDirectory);

        setModificationTime(oldCandidate, Old);
        setModificationTime(oldTemporary, Old);
        setModificationTime(youngCandidate, Young);
        setModificationTime(uppercaseHash, Old);
        setModificationTime(leadingZero, Old);
        setModificationTime(foreignTemporary, Old);
        setModificationTime(candidateSymlink, Old);
        setModificationTime(candidateDirectory, Old);

        TmdbSeriesArtworkIncomingCleaner cleaner(enabledConfig(root));
        const auto result = cleaner.cleanup(Now);
        assert(result.attempted);
        assert(result.rootAvailable);
        assert(result.succeeded());
        assert(result.removedCandidateFiles == 1U);
        assert(result.removedTemporaryFiles == 1U);
        assert(result.youngFiles == 1U);
        assert(result.skippedForeignEntries >= 3U);
        assert(result.skippedUnsafeEntries == 2U);
        assert(!std::filesystem::exists(oldCandidate));
        assert(!std::filesystem::exists(oldTemporary));
        assert(std::filesystem::exists(youngCandidate));
        assert(std::filesystem::exists(uppercaseHash));
        assert(std::filesystem::exists(leadingZero));
        assert(std::filesystem::exists(foreignTemporary));
        assert(std::filesystem::is_symlink(candidateSymlink));
        assert(std::filesystem::is_directory(candidateDirectory));
        assert(std::filesystem::exists(external));

        std::filesystem::remove_all(root);
        std::filesystem::remove(external);
    }

    {
        const std::filesystem::path realRoot = temporaryDirectory("incoming-real");
        const std::filesystem::path oldCandidate =
            realRoot / "tmdb-100-2222222222222222.candidate";
        writeFile(oldCandidate);
        setModificationTime(oldCandidate, Old);

        const std::filesystem::path symlinkRoot = realRoot.parent_path() /
            (realRoot.filename().string() + "-link");
        std::filesystem::create_directory_symlink(realRoot, symlinkRoot);

        TmdbSeriesArtworkIncomingCleaner cleaner(enabledConfig(symlinkRoot));
        const auto result = cleaner.cleanup(Now);
        assert(result.attempted);
        assert(!result.rootAvailable);
        assert(!result.succeeded());
        assert(result.errors == 1U);
        assert(std::filesystem::exists(oldCandidate));

        std::filesystem::remove(symlinkRoot);
        std::filesystem::remove_all(realRoot);
    }

    {
        const std::filesystem::path root = temporaryDirectory("incoming-batch");
        const std::filesystem::path first =
            root / "tmdb-1-0000000000000001.candidate";
        const std::filesystem::path second =
            root / "tmdb-2-0000000000000002.candidate";
        const std::filesystem::path third =
            root / "tmdb-3-0000000000000003.candidate";
        for (const auto& path : {first, second, third})
        {
            writeFile(path);
            setModificationTime(path, Old);
        }

        TmdbSeriesArtworkIncomingCleaner cleaner(enabledConfig(root, 2U));
        const auto firstResult = cleaner.cleanup(Now);
        assert(firstResult.succeeded());
        assert(firstResult.removedFiles() == 2U);
        assert(firstResult.limitReached);
        assert(!std::filesystem::exists(first));
        assert(!std::filesystem::exists(second));
        assert(std::filesystem::exists(third));

        const auto secondResult = cleaner.cleanup(Now);
        assert(secondResult.succeeded());
        assert(secondResult.removedFiles() == 1U);
        assert(!secondResult.limitReached);
        assert(!std::filesystem::exists(third));

        std::filesystem::remove_all(root);
    }

    {
        auto config = enabledConfig("/tmp/incoming");
        config.incomingRoot = "/tmp/../tmp/incoming";
        assert(!config.valid());
        config = enabledConfig("/tmp/incoming", 1025U);
        assert(!config.valid());
        config = enabledConfig("/tmp/incoming");
        config.minimumAgeSeconds = 0;
        assert(!config.valid());
    }

    return 0;
}
