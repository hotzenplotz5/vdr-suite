#include "DaemonMediaRecordingSourceResolution.h"

#include <cassert>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>
#include <unistd.h>

namespace fs = std::filesystem;

int main()
{
    const std::string pattern =
        (fs::temp_directory_path() / "vdr-suite-media-root-XXXXXX").string();
    std::vector<char> name(pattern.begin(), pattern.end());
    name.push_back('\0');
    const char* made = ::mkdtemp(name.data());
    assert(made != nullptr);
    const fs::path home(made);
    struct Cleanup {
        fs::path path;
        ~Cleanup() { std::error_code error; fs::remove_all(path, error); }
    } cleanup{home};

    // Distribution-neutral: neither the configured root nor its VDR alias
    // is tied to a known operating-system or package installation path.
    const fs::path root = home / "distribution" / "vdr-recordings";
    const fs::path relative =
        fs::path("Doku") / "Film" / "2026-11-11.12.00.1-0.rec";
    fs::create_directories(root / relative);
    const fs::path alias = home / "provider-vdr-root";
    fs::create_directory_symlink(root, alias);

    VdrRecording recording;
    recording.backendId = "backend-a";
    recording.path = "/" + relative.string();  // RESTfulAPI display-relative
    recording.backendNativeId = (alias / relative).string();
    const auto resolved = vdrsuite::mediatools::
        resolveConfiguredRecordingPath(root.string(), recording);
    assert(resolved && *resolved == root / relative);

    // The provider also works when it reports the direct native root.
    recording.backendNativeId = (root / relative).string();
    assert(vdrsuite::mediatools::resolveConfiguredRecordingPath(
        root.string(), recording) == root / relative);

    // A recording from an unrelated mount must never be admitted.
    const fs::path elsewhere = home / "another-mount" / relative;
    fs::create_directories(elsewhere);
    recording.backendNativeId = elsewhere.string();
    assert(!vdrsuite::mediatools::resolveConfiguredRecordingPath(
        root.string(), recording));

    // A provider alias to the root is allowed, but a symlink *within* the
    // recording tree is not: the normal Check/Shrink symlink policy applies.
    fs::create_directory_symlink(root / "Doku", root / "linked-folder");
    recording.backendNativeId = (
        alias / "linked-folder" / "Film" / relative.filename()).string();
    assert(!vdrsuite::mediatools::resolveConfiguredRecordingPath(
        root.string(), recording));

    // Even a valid native recording cannot authorize a symlinked configured
    // video root or a non-normalized VDR source path.
    recording.backendNativeId = (alias / relative).string();
    assert(!vdrsuite::mediatools::resolveConfiguredRecordingPath(
        alias.string(), recording));
    recording.backendNativeId =
        (alias / "Doku" / ".." / relative).string();
    assert(!vdrsuite::mediatools::resolveConfiguredRecordingPath(
        root.string(), recording));

    recording.backendNativeId.clear();
    assert(!vdrsuite::mediatools::resolveConfiguredRecordingPath(
        root.string(), recording));

    std::cout << "MEDIA_CHECK_SHRINK_CACHE_PATH_RESOLUTION=PASS\n";
}
