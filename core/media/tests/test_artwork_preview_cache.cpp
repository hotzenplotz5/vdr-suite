#include "ArtworkPreviewCache.h"
#include "MediaProcessRunner.h"
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sys/stat.h>
#include <unistd.h>

namespace fs = std::filesystem;
std::string fixture(const std::string& codec, const std::string& size = "800x1200") {
    const auto image = MediaProcessRunner{}.runAndCapture({"/usr/bin/ffmpeg", "-nostdin", "-v", "error",
        "-f", "lavfi", "-i", "testsrc2=size=" + size, "-frames:v", "1", "-threads", "1",
        "-c:v", codec, "-f", "image2pipe", "pipe:1"}, "/", std::chrono::seconds(10), 16 * 1024 * 1024);
    assert(image.success && !image.output.empty());
    return image.output;
}
void write(const fs::path& path, const std::string& bytes) {
    std::ofstream out(path, std::ios::binary); out << bytes; assert(out.good());
}
int main() {
    char temporary[] = "/tmp/vdr-preview-test-XXXXXX";
    assert(::mkdtemp(temporary));
    const fs::path root(temporary), cacheRoot = root / "cache";
    ArtworkPreviewCache cache(cacheRoot.string());
    const auto png = fixture("png");
    const auto preview = cache.home("recording-1?assignmentRevision=1", "image/png", png);
    assert(!preview.empty() && preview.size() < png.size());
    assert(static_cast<unsigned char>(preview[0]) == 255 && preview[1] == '\xd8');
    write(root / "preview.jpg", preview);
    auto probe = MediaProcessRunner{}.runAndCapture({"/usr/bin/ffprobe", "-v", "error",
        "-show_entries", "stream=width,height", "-of", "csv=p=0", (root / "preview.jpg").string()},
        "/", std::chrono::seconds(5), 100);
    assert(probe.success && probe.output == "400,600\n");
    // Restart with no decoder: cache hit proves persistence and no re-encoding.
    ArtworkPreviewCache restarted(cacheRoot.string(), "/missing/ffmpeg");
    assert(restarted.home("recording-1?assignmentRevision=1", "image/png", png) == preview);
    assert(restarted.home("recording-1?assignmentRevision=2", "image/png", png).empty());
    const auto changed = fixture("png", "1000x700");
    assert(restarted.home("recording-1?assignmentRevision=1", "image/png", changed).empty());
    assert(cache.home("small", "image/png", fixture("png", "200x300")).empty());
    assert(cache.home("wrong-type", "image/jpeg", png).empty());
    assert(cache.home("unsupported", "image/svg+xml", "<svg/>").empty());
    assert(cache.home("broken", "image/jpeg", "\xff\xd8\xff").empty());
    assert(cache.home("oversized", "image/png", std::string(16 * 1024 * 1024 + 1, 'x')).empty());
    auto bomb = png;
    bomb[16] = 0x7f; // Header exceeds pixel and dimension bounds; no decode.
    assert(cache.home("bomb", "image/png", bomb).empty());
    for (const auto& item : {std::make_pair("mjpeg", "image/jpeg"), std::make_pair("libwebp", "image/webp")}) {
        const auto bytes = fixture(item.first, "1200x800");
        const auto result = cache.home(item.first, item.second, bytes);
        assert(!result.empty() && result.size() < bytes.size());
        write(root / "landscape.jpg", result);
        probe = MediaProcessRunner{}.runAndCapture({"/usr/bin/ffprobe", "-v", "error", "-show_entries",
            "stream=width,height", "-of", "csv=p=0", (root / "landscape.jpg").string()},
            "/", std::chrono::seconds(5), 100);
        assert(probe.success && probe.output == "400,266\n");
    }
    fs::create_directory_symlink(cacheRoot, root / "link");
    assert(ArtworkPreviewCache((root / "link").string()).home("link", "image/png", png).empty());
    // Cache entry symlinks cannot expose arbitrary files, even on a cache hit.
    for (const auto& entry : fs::directory_iterator(cacheRoot)) {
        fs::remove(entry.path());
        fs::create_symlink(root / "preview.jpg", entry.path());
    }
    assert(restarted.home("recording-1?assignmentRevision=1", "image/png", png).empty());
    fs::remove_all(cacheRoot);
    fs::create_directory(cacheRoot);
    ::chmod(cacheRoot.c_str(), 0700);
    // Capacity is enforced before publishing the next output.
    for (unsigned i = 0; i < 256; ++i) {
        char name[69];
        std::snprintf(name, sizeof(name), "%064x.jpg", i);
        write(cacheRoot / name, std::string(160 * 1024, 'x'));
    }
    assert(!cache.home("eviction", "image/png", png).empty());
    std::size_t count = 0, bytes = 0;
    for (const auto& entry : fs::directory_iterator(cacheRoot)) { ++count; bytes += entry.file_size(); }
    assert(count <= 256 && bytes <= 32 * 1024 * 1024);
    // A timed-out decoder is killed; a repeated failed identity is cooled down.
    write(root / "slow", "#!/bin/sh\nsleep 10\n");
    ::chmod((root / "slow").c_str(), 0700);
    ArtworkPreviewCache slow((root / "slow-cache").string(), (root / "slow").string());
    auto start = std::chrono::steady_clock::now();
    assert(slow.home("slow", "image/png", png).empty());
    assert(std::chrono::steady_clock::now() - start < std::chrono::seconds(5));
    start = std::chrono::steady_clock::now();
    assert(slow.home("slow", "image/png", png).empty());
    assert(std::chrono::steady_clock::now() - start < std::chrono::seconds(1));
    fs::remove_all(root);
    std::cout << "artwork preview dimensions, formats, persistence, revision, bounds, symlink safety and timeout ok\n";
}
