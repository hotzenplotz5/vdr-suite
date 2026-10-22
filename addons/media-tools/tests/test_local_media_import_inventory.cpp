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
    assert(inventory.listTrustedSourceIds("other", ids));
    assert(ids.empty());
    assert(!inventory.resolve("other", Key, selected));
    assert(selected.sourceFile.empty());
    assert(!inventory.resolve("backend", "../../etc/passwd", selected));
    assert(!inventory.resolve("backend", "short", selected));

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
    fs::last_write_time(newFile,
        fs::file_time_type::clock::now() - std::chrono::minutes(10));
    fs::last_write_time(fixture.source,
        fs::file_time_type::clock::now() - std::chrono::minutes(10));
    fixture.write(std::string("backend\t") + Key + "\t" +
        (fixture.root / "media/input").string() + "\t" +
        fixture.destination.string() + "\t*\n");
    assert(inventory.listTrustedSourceIds("backend", ids));
    assert(ids.size() == 2);
    assert(ids[0].rfind("scan_", 0) == 0 &&
           ids[1].rfind("scan_", 0) == 0);
    LocalMediaImportSource scanned;
    assert(inventory.resolve("backend", ids[0], scanned));
    assert(scanned.sourceFile == fixture.source ||
           scanned.sourceFile == newFile);
    const auto previous = ids;
    assert(inventory.listTrustedSourceIds("backend", ids) && ids == previous);
    fs::last_write_time(newFile, fs::file_time_type::clock::now());
    assert(inventory.listTrustedSourceIds("backend", ids) && ids.size() == 1);
    assert(!inventory.resolve("backend", previous[0], scanned) ||
           scanned.sourceFile == fixture.source);
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
