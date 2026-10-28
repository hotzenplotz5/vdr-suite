#include "LocalMediaImportInventory.h"

#include <cassert>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <grp.h>
#include <pwd.h>
#include <stdexcept>
#include <string>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

namespace fs = std::filesystem;
using namespace vdrsuite::mediatools;

namespace {
constexpr const char* Key = "importToken_A6_0001";
struct Fixture {
    fs::path root, source, destination, ledger;
    Fixture()
    {
        if (::geteuid() != 0 || !std::getenv("TMPDIR") ||
            std::string(std::getenv("TMPDIR")) != "/root")
            throw std::runtime_error("isolated root CI only");
        struct stat run{};
        if (::stat("/run", &run) != 0 || !S_ISDIR(run.st_mode) ||
            run.st_uid != 0 || (run.st_mode & 0022))
            throw std::runtime_error("unsafe /run parent");
        std::string pattern = "/run/vdr-a6-sources-XXXXXX";
        std::vector<char> name(pattern.begin(), pattern.end());
        name.push_back('\0');
        if (!::mkdtemp(name.data())) throw std::runtime_error("mkdtemp");
        root = name.data();
        source = root / "media/input/clip.ts";
        destination = root / "media/video";
        ledger = root / "etc/vdr-suite/addon-sources/media-import.tsv";
        fs::create_directories(source.parent_path());
        fs::create_directories(destination);
        fs::create_directories(ledger.parent_path());
        for (const auto& relative : {"", "media", "media/input", "media/video",
            "etc", "etc/vdr-suite", "etc/vdr-suite/addon-sources"})
        {
            if (::chmod((root / relative).c_str(), 0755) != 0)
                throw std::runtime_error("fixture directory permissions");
        }
        { std::ofstream data(source); data << "synthetic-transport-stream"; }
        if (::chmod(source.c_str(), 0644) != 0)
            throw std::runtime_error("fixture source permissions");
        write();
    }
    std::string entry() const
    {
        return std::string("backend\t") + Key + "\t" +
            (root / "media/input").string() + "\t" +
            destination.string() + "\t" + source.string() + "\n";
    }
    void write(const std::string& value = "")
    {
        { std::ofstream file(ledger, std::ios::trunc);
          file << (value.empty() ? entry() : value); }
        if (::chmod(ledger.c_str(), 0644) != 0)
            throw std::runtime_error("fixture ledger permissions");
    }
    ~Fixture()
    {
        std::error_code ec;
        fs::remove_all(root, ec);
    }
};
}

int main()
{
    Fixture fixture;
    RootOwnedMediaImportInventory inventory(fixture.root);
    LocalMediaImportSource selected;
    assert(inventory.resolve("backend", Key, selected));
    assert(selected.sourceFile == fixture.source);
    assert(selected.videoRoot == fixture.destination);
    assert(selected.sourceDevice != 0 && selected.sourceInode != 0);
    assert(selected.sourceBytes > 0);
    std::vector<std::string> ids;
    assert(inventory.listTrustedSourceIds("backend", ids));
    assert(ids == std::vector<std::string>{Key});
    std::vector<MediaImportSourceLabel> labels;
    bool partial = true;
    assert(inventory.listTrustedSourceLabels("backend", labels));
    assert(labels.size() == 1 && labels[0].sourceId == Key &&
           labels[0].label == "clip");
    assert(inventory.listTrustedSourceIds("other", ids));
    assert(ids.empty());
    assert(!inventory.resolve("other", Key, selected));
    assert(selected.sourceFile.empty());
    assert(!inventory.resolve("backend", "../../etc/passwd", selected));
    assert(!inventory.resolve("backend", "short", selected));

    // The observation returns one chosen descriptor from the SAME inventory
    // pass as its labels, not a second source-tree resolution.
    LocalMediaImportSource observed;
    assert(inventory.listTrustedSourceSnapshot(
        "backend", labels, partial, Key, &observed));
    assert(!partial && labels.size() == 1 && labels[0].sourceId == Key);
    assert(observed.sourceFile == fixture.source &&
           observed.sourceInode != 0 && observed.sourceBytes > 0);
    assert(inventory.listTrustedSourceSnapshot(
        "backend", labels, partial, "missing_Token_0001", &observed));
    assert(observed.sourceFile.empty() && labels.size() == 1);

    // MT.UX2: root-owned optional sixth column names each source root.
    // The first matching backend entry becomes the default for display.
    const auto extraDir = fixture.root / "media/extra";
    fs::create_directories(extraDir);
    const auto extraFile = extraDir / "zweiter_film.mkv";
    { std::ofstream out(extraFile); out << "synthetic-secondary-media"; }
    const std::string firstAlias = fixture.entry().substr(
        0, fixture.entry().size() - 1) + "\tFilme\n";
    const std::string secondAlias =
        std::string("backend\timportToken_A6_0002\t") +
        extraDir.string() + "\t" + fixture.destination.string() +
        "\t" + extraFile.string() + "\tSerien\n";
    fixture.write(firstAlias + secondAlias);
    assert(inventory.listTrustedSourceSnapshot("backend", labels, partial));
    assert(!partial && labels.size() == 2);
    assert(labels[0].label == "Standard: Filme · clip");
    assert(labels[1].label == "Serien · zweiter_film");
    assert(labels[0].folder == "Standard: Filme");
    assert(labels[1].folder == "Serien");
    assert(inventory.resolve("backend", "importToken_A6_0002", selected));
    assert(selected.sourceFile == extraFile);
    fixture.write(secondAlias + firstAlias);
    assert(inventory.listTrustedSourceLabels("backend", labels));
    assert(labels.size() == 2);
    assert(labels[0].label == "Filme · clip");
    assert(labels[1].label == "Standard: Serien · zweiter_film");
    assert(labels[0].folder == "Filme");
    assert(labels[1].folder == "Standard: Serien");
    // An alias never authorizes a different backend.
    assert(inventory.listTrustedSourceIds("other", ids) && ids.empty());
    fixture.write(firstAlias + secondAlias +
        "other\timportToken_A6_0003\t" + extraDir.string() + "\t" +
        fixture.destination.string() + "\t" + extraFile.string() +
        "\tAndere\n");
    assert(inventory.listTrustedSourceIds("backend", ids) && ids.size() == 2);
    assert(inventory.listTrustedSourceIds("other", ids) && ids.size() == 1);
    // Display metadata must not accept path separators or controls.
    fixture.write(fixture.entry().substr(0, fixture.entry().size() - 1) +
        "\tVerboten/Quelle\n");
    assert(!inventory.listTrustedSourceLabels("backend", labels));
    assert(labels.empty());
    fixture.write(firstAlias + secondAlias + firstAlias);
    assert(!inventory.listTrustedSourceIds("backend", ids));
    fixture.write();

    // All duplicate and malformed rows fail closed, including unrelated rows.
    fixture.write(fixture.entry() + fixture.entry());
    assert(!inventory.resolve("backend", Key, selected));
    assert(!inventory.listTrustedSourceIds("backend", ids));
    assert(ids.empty());
    fixture.write(fixture.entry() + "malformed\n");
    assert(!inventory.resolve("backend", Key, selected));
    assert(!inventory.listTrustedSourceIds("backend", ids));
    fixture.write();

    // No writable or symlinked authority, even with syntactically valid keys.
    assert(::chmod(fixture.ledger.c_str(), 0666) == 0);
    assert(!inventory.resolve("backend", Key, selected));
    assert(!inventory.listTrustedSourceIds("backend", ids));
    assert(::chmod(fixture.ledger.c_str(), 0644) == 0);
    const auto originalLedger = fixture.ledger.string() + ".saved";
    fs::rename(fixture.ledger, originalLedger);
    fs::create_symlink(originalLedger, fixture.ledger);
    assert(!inventory.resolve("backend", Key, selected));
    fs::remove(fixture.ledger);
    fs::rename(originalLedger, fixture.ledger);

    // The canonical C++ importer already rejects traversal and symlinks.
    fixture.write(std::string("backend\t") + Key + "\t" +
        (fixture.root / "media/input").string() + "\t" +
        fixture.destination.string() + "\t" +
        (fixture.root / "media/input/../video/clip.ts").string() + "\n");
    assert(!inventory.resolve("backend", Key, selected));
    fixture.write();
    fs::rename(fixture.source, fixture.source.string() + ".saved");
    fs::create_symlink(fixture.source.string() + ".saved", fixture.source);
    assert(!inventory.resolve("backend", Key, selected));
    fs::remove(fixture.source);
    fs::rename(fixture.source.string() + ".saved", fixture.source);

    // One root-owned directory directive discovers both direct and nested
    // stable sources, without a client-supplied filesystem path.
    fs::create_directories(fixture.root / "media/input/subfolder");
    const auto newFile = fixture.root / "media/input/subfolder/new_movie.webm";
    { std::ofstream out(newFile); out << "webm payload"; }
    const auto seriesDir = fixture.root / "media/input/Serien/Star Wars-Andor";
    fs::create_directories(seriesDir);
    const auto seriesFile = seriesDir / "episode_1.mkv";
    { std::ofstream out(seriesFile); out << "synthetic-series-media"; }
    // MT.UX1: fully nested source folders must be inspected read-only,
    // without a depth-2 shortcut or browser-supplied absolute path.
    const auto deepDir = seriesDir / "Staffel 1/Extras/Lange Reihe";
    fs::create_directories(deepDir);
    const auto deepFile = deepDir / "episode_deep.mkv";
    { std::ofstream out(deepFile); out << "synthetic-deep-series-media"; }
    // Directory symlinks remain prohibited, including links to valid media.
    fs::create_directory_symlink(deepDir, fixture.root / "media/input/Falscher Pfad");
    fs::last_write_time(seriesFile,
        fs::file_time_type::clock::now() - std::chrono::minutes(10));
    fs::last_write_time(deepFile,
        fs::file_time_type::clock::now() - std::chrono::minutes(10));
    fs::last_write_time(newFile,
        fs::file_time_type::clock::now() - std::chrono::minutes(10));
    fs::last_write_time(fixture.source,
        fs::file_time_type::clock::now() - std::chrono::minutes(10));
    fixture.write(std::string("backend\t") + Key + "\t" +
        (fixture.root / "media/input").string() + "\t" +
        fixture.destination.string() + "\t*\n");
    assert(inventory.listTrustedSourceIds("backend", ids));
    assert(ids.size() == 4);
    for (const auto& id : ids)
        assert(id.rfind("scan_", 0) == 0);
    // The productive wildcard scan, not only explicitly named files,
    // must propagate the trusted source-root alias.
    const std::string wildcardRow =
        std::string("backend\t") + Key + "\t" +
        (fixture.root / "media/input").string() + "\t" +
        fixture.destination.string() + "\t*";
    fixture.write(wildcardRow + "\tFilme\n");
    assert(inventory.listTrustedSourceSnapshot("backend", labels, partial));
    assert(!partial && labels.size() == 4);
    bool sawNestedFolder = false;
    bool sawRootFolder = false;
    for (const auto& item : labels) {
        assert(item.label.rfind("Standard: Filme · ", 0) == 0);
        if (item.label == "Standard: Filme · episode_deep") {
            assert(item.folder ==
                "Serien/Star Wars-Andor/Staffel 1/Extras/Lange Reihe");
            sawNestedFolder = true;
        }
        if (item.label == "Standard: Filme · clip") {
            assert(item.folder.empty());
            sawRootFolder = true;
        }
    }
    assert(sawNestedFolder && sawRootFolder);
    fixture.write(wildcardRow + "\n");
    bool sawSeries = false;
    bool sawDeep = false;
    std::string newMovieId;
    for (const auto& id : ids) {
        LocalMediaImportSource row;
        assert(inventory.resolve("backend", id, row));
        sawSeries |= row.sourceFile == seriesFile;
        sawDeep |= row.sourceFile == deepFile;
        assert(row.sourceFile != fixture.root / "media/input/Falscher Pfad/episode_deep.mkv");
        if (row.sourceFile == newFile) newMovieId = id;
    }
    assert(sawSeries && sawDeep && !newMovieId.empty());
    assert(inventory.listTrustedSourceSnapshot(
        "backend", labels, partial, newMovieId, &observed));
    assert(!partial && labels.size() == 4 && observed.sourceFile == newFile);
    LocalMediaImportSource scanned;
    assert(inventory.resolve("backend", ids[0], scanned));
    assert(scanned.sourceFile == fixture.source ||
           scanned.sourceFile == newFile ||
           scanned.sourceFile == seriesFile ||
           scanned.sourceFile == deepFile);
    // MT.UX1 bounded catalog regression: all 74 eligible files are visible,
    // not silently truncated at the former 32-ID Agent ceiling.
    const auto bulkDir = fixture.root / "media/input/Bulk";
    fs::create_directory(bulkDir);
    for (int index = 0; index < 70; ++index) {
        const auto clip = bulkDir /
            ("episode_" + std::to_string(index) + ".mkv");
        { std::ofstream out(clip); out << "synthetic-bulk-media"; }
        fs::last_write_time(clip,
            fs::file_time_type::clock::now() - std::chrono::minutes(10));
    }
    assert(inventory.listTrustedSourceIds("backend", ids));
    assert(ids.size() == 74);
    assert(inventory.listTrustedSourceLabels("backend", labels));
    assert(labels.size() == 74);
    bool sawBulk = false;
    for (const auto& id : ids) {
        LocalMediaImportSource source;
        assert(inventory.resolve("backend", id, source));
        if (source.sourceFile == bulkDir / "episode_69.mkv") sawBulk = true;
    }
    assert(sawBulk);
    partial = true;
    assert(inventory.listTrustedSourceSnapshot("backend", labels, partial));
    assert(!partial && labels.size() == 74);

    // Exactly 100 candidates remain a complete scan (4 + 96).
    for (int index = 70; index < 96; ++index) {
        const auto clip = bulkDir / ("episode_" + std::to_string(index) + ".mkv");
        { std::ofstream out(clip); out << "synthetic-bulk-media"; }
        fs::last_write_time(clip,
            fs::file_time_type::clock::now() - std::chrono::minutes(10));
    }
    assert(inventory.listTrustedSourceSnapshot("backend", labels, partial));
    assert(!partial && labels.size() == 100);
    // The 101st eligible file MUST not appear, and overflow is explicit.
    for (int index = 96; index < 270; ++index) {
        const auto clip = bulkDir /
            ("episode_" + std::to_string(index) + ".mkv");
        { std::ofstream out(clip); out << "synthetic-bulk-media"; }
        fs::last_write_time(clip,
            fs::file_time_type::clock::now() - std::chrono::minutes(10));
    }
    assert(inventory.listTrustedSourceSnapshot("backend", labels, partial));
    assert(partial && labels.size() == 100);
    assert(inventory.listTrustedSourceIds("backend", ids) && ids.size() == 100);
    fs::remove_all(bulkDir);

    // Directory-entry overflow is a PARTIAL read, not an empty/falsely
    // complete catalog. Non-media entries can reach this bound as well.
    const auto huge = fixture.root / "media/input/ManyEntries";
    fs::create_directory(huge);
    for (int index = 0; index < 4100; ++index)
        { std::ofstream out(huge / ("unused_" + std::to_string(index) + ".txt")); }
    assert(inventory.listTrustedSourceSnapshot("backend", labels, partial));
    assert(partial);
    assert(labels.size() <= 100);
    fs::remove_all(huge);
    assert(inventory.listTrustedSourceSnapshot("backend", labels, partial));
    assert(!partial && labels.size() == 4);
    assert(inventory.listTrustedSourceIds("backend", ids));
    assert(ids.size() == 4);
    const auto previous = ids;
    assert(inventory.listTrustedSourceIds("backend", ids) && ids == previous);
    fs::last_write_time(newFile, fs::file_time_type::clock::now());
    assert(inventory.listTrustedSourceIds("backend", ids) && ids.size() == 3);
    assert(!inventory.resolve("backend", newMovieId, scanned));
    assert(inventory.listTrustedSourceSnapshot(
        "backend", labels, partial, newMovieId, &observed));
    assert(observed.sourceFile.empty() && labels.size() == 3);
    fixture.write();
    // A root-approved marked source is observable without becoming importable.
    const auto held = fixture.source.parent_path() / "clip.pc_encode.ts";
    fs::rename(fixture.source, held);
    fixture.write(std::string("backend\t") + Key + "\t" +
        (fixture.root / "media/input").string() + "\t" +
        fixture.destination.string() + "\t" + held.string() + "\n");
    assert(inventory.resolve("backend", Key, selected));
    assert(selected.sourceFile == held);
    assert(inventory.listTrustedSourceLabels("backend", labels));
    assert(labels.size() == 1 && labels[0].label == "clip.pc_encode");
    fs::rename(held, fixture.source);
    fixture.write();

    // A complete pre-existing VDR recording must not make an unmarked
    // import source disappear from either the explicit or recursive catalog.
    // The productive inspector confirms the duplicate before any marker
    // operation is allowed; scanning itself remains read-only.
    const auto targetRec =
        fixture.destination / "clip" / "2026-01-01.00.00.1-0.rec";
    fs::create_directories(targetRec);
    std::ofstream(targetRec / "info") << "T clip\n";
    std::ofstream(targetRec / "index") << "synthetic index";
    std::ofstream(targetRec / "00001.ts") << "synthetic video";
    assert(inventory.resolve("backend", Key, selected) &&
           selected.sourceFile == fixture.source);
    fixture.write(wildcardRow + "\n");
    assert(inventory.listTrustedSourceIds("backend", ids) && ids.size() == 3);
    bool duplicateVisible = false;
    for (const auto& id : ids) {
        assert(inventory.resolve("backend", id, selected));
        duplicateVisible |= selected.sourceFile == fixture.source;
    }
    assert(duplicateVisible);
    fixture.write();

    // Root-owned inventory is readable by the real unprivileged Agent uid.
    const passwd* nobody = ::getpwnam("nobody");
    assert(nobody != nullptr && nobody->pw_uid != 0);
    const pid_t child = ::fork();
    assert(child >= 0);
    if (child == 0)
    {
        if (::setgroups(0, nullptr) || ::setgid(nobody->pw_gid) ||
            ::setuid(nobody->pw_uid))
            ::_exit(2);
        RootOwnedMediaImportInventory nonroot(fixture.root);
        LocalMediaImportSource result;
        std::vector<std::string> safeIds;
        const bool canRead = nonroot.resolve("backend", Key, result) &&
            result.sourceFile == fixture.source &&
            nonroot.listTrustedSourceIds("backend", safeIds) &&
            safeIds == std::vector<std::string>{Key};
        ::_exit(canRead ? 0 : 3);
    }
    int status = 0;
    assert(::waitpid(child, &status, 0) == child);
    assert(WIFEXITED(status) && WEXITSTATUS(status) == 0);
}
