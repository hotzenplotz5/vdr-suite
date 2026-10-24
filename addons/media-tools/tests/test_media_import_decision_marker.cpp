#include "MediaImportDecisionMarker.h"

#include <cassert>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

namespace fs = std::filesystem;
using namespace vdrsuite::mediatools;

struct Temp {
    fs::path path;
    Temp() {
        const char* dir = std::getenv("TMPDIR");
        if (!dir || !*dir) throw std::runtime_error("isolated TMPDIR required");
        std::string name = (fs::path(dir) / "media-tools-mark-XXXXXX").string();
        std::vector<char> bytes(name.begin(), name.end());
        bytes.push_back('\0');
        char* created = ::mkdtemp(bytes.data());
        if (!created) throw std::runtime_error("mkdtemp failed");
        path = created;
    }
    ~Temp() {
        std::error_code error;
        fs::remove_all(path, error);
    }
};
void write(const fs::path& p, const std::string& content = "unchanged media bytes") {
    std::ofstream file(p, std::ios::binary | std::ios::trunc);
    file << content;
    file.close();
    assert(fs::file_size(p) == content.size());
}
LocalMediaImportSource source(const fs::path& root, const fs::path& video,
                              const fs::path& file) {
    struct stat st{};
    assert(::lstat(file.c_str(), &st) == 0);
    LocalMediaImportSource result;
    result.sourceRoot = root;
    result.videoRoot = video;
    result.sourceFile = file;
    result.sourceDevice = st.st_dev;
    result.sourceInode = st.st_ino;
    result.sourceBytes = st.st_size;
    result.sourceMtimeSeconds = st.st_mtim.tv_sec;
    result.sourceMtimeNanoseconds = st.st_mtim.tv_nsec;
    return result;
}
std::string contents(const fs::path& path) {
    std::ifstream in(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}

int main() {
    Temp t;
    const auto root = t.path / "sources", video = t.path / "video";
    fs::create_directories(root / "folder");
    fs::create_directories(video);
    const auto original = root / "folder" / "Die Herrschaft des Feuers.mkv";
    const auto deferred = root / "folder" / "Die Herrschaft des Feuers.pc_encode.mkv";
    const auto skipped = root / "folder" / "Die Herrschaft des Feuers.skipped.mkv";
    write(original);
    const auto identity = source(root, video, original);
    assert(markMediaImportSource(identity, MediaImportMarkerDecision::desktop) ==
           MediaImportMarkerOutcome::marked);
    assert(!fs::exists(original));
    assert(contents(deferred) == "unchanged media bytes");
    const auto held = source(root, video, deferred);
    assert(markMediaImportSource(identity, MediaImportMarkerDecision::release) ==
           MediaImportMarkerOutcome::rejected);
    assert(markMediaImportSource(held, MediaImportMarkerDecision::release) ==
           MediaImportMarkerOutcome::marked);
    assert(contents(original) == "unchanged media bytes");
    assert(markMediaImportSource(source(root, video, original),
           MediaImportMarkerDecision::desktop) == MediaImportMarkerOutcome::marked);
    assert(markMediaImportSource(identity, MediaImportMarkerDecision::skip) ==
           MediaImportMarkerOutcome::rejected);  // old opaque inventory ID is stale.
    assert(contents(deferred) == "unchanged media bytes");

    const auto second = root / "folder" / "Another Movie.mp4";
    const auto secondSkipped = root / "folder" / "Another Movie.skipped.mp4";
    write(second);
    const auto secondIdentity = source(root, video, second);
    write(secondSkipped, "do not overwrite");
    assert(markMediaImportSource(secondIdentity, MediaImportMarkerDecision::skip) ==
           MediaImportMarkerOutcome::rejected);
    assert(contents(second) == "unchanged media bytes");
    assert(contents(secondSkipped) == "do not overwrite");
    fs::remove(secondSkipped);
    assert(markMediaImportSource(secondIdentity, MediaImportMarkerDecision::skip) ==
           MediaImportMarkerOutcome::marked);
    assert(!fs::exists(second));
    assert(contents(secondSkipped) == "unchanged media bytes");
    write(second, "existing target");
    assert(markMediaImportSource(source(root, video, secondSkipped),
           MediaImportMarkerDecision::release) == MediaImportMarkerOutcome::rejected);
    assert(contents(second) == "existing target");
    assert(contents(secondSkipped) == "unchanged media bytes");
    fs::remove(second);
    assert(markMediaImportSource(source(root, video, secondSkipped),
           MediaImportMarkerDecision::release) == MediaImportMarkerOutcome::marked);
    assert(contents(second) == "unchanged media bytes");
    assert(!fs::exists(secondSkipped));
    assert(markMediaImportSource(source(root, video, second),
           MediaImportMarkerDecision::skip) == MediaImportMarkerOutcome::marked);

    // No action on already marked or duplicated media.
    assert(markMediaImportSource(source(root, video, secondSkipped),
           MediaImportMarkerDecision::desktop) ==
           MediaImportMarkerOutcome::rejected);
    const auto duplicate = root / "folder" / "Old.duplicate.mkv";
    write(duplicate);
    assert(markMediaImportSource(source(root, video, duplicate),
           MediaImportMarkerDecision::skip) ==
           MediaImportMarkerOutcome::rejected);
    assert(markMediaImportSource(source(root, video, duplicate),
           MediaImportMarkerDecision::release) ==
           MediaImportMarkerOutcome::marked);
    assert(!fs::exists(duplicate));
    assert(contents(root / "folder/Old.mkv") == "unchanged media bytes");

    // Stale source identity can never rename a newly replaced file.
    const auto changed = root / "folder" / "Stale.ts";
    write(changed);
    const auto stale = source(root, video, changed);
    write(changed, "changed contents and size");
    assert(markMediaImportSource(stale, MediaImportMarkerDecision::skip) ==
           MediaImportMarkerOutcome::rejected);
    assert(fs::exists(changed));

    // Symlinks in both the source itself and all ancestors are rejected.
    const auto target = root / "folder" / "Safe.mkv";
    write(target);
    const auto symlinkFile = root / "folder" / "Unsafe.mkv";
    fs::create_symlink(target, symlinkFile);
    auto forged = source(root, video, target);
    forged.sourceFile = symlinkFile;
    assert(markMediaImportSource(forged, MediaImportMarkerDecision::desktop) ==
           MediaImportMarkerOutcome::rejected);
    fs::create_directory_symlink(root / "folder", root / "alias");
    forged.sourceFile = root / "alias" / "Safe.mkv";
    assert(markMediaImportSource(forged, MediaImportMarkerDecision::desktop) ==
           MediaImportMarkerOutcome::rejected);
    assert(fs::exists(target));

    // A file outside the approved root cannot be marked.
    forged = source(root, video, target);
    forged.sourceRoot = video;
    assert(markMediaImportSource(forged, MediaImportMarkerDecision::skip) ==
           MediaImportMarkerOutcome::rejected);
    assert(fs::exists(target));
    std::cout << "MEDIA_TOOLS_DECISION_MARKERS=PASS\n";
}
