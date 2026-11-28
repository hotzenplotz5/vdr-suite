#include "LocalImageGalleryProvider.h"

#include <cassert>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sys/stat.h>

namespace fs = std::filesystem;
using namespace vdrsuite::imagegallery;

struct Fixture {
    fs::path path;
    Fixture() {
        auto pattern = (fs::temp_directory_path() / "suite-image-XXXXXX").string();
        const auto* result = ::mkdtemp(pattern.data());
        assert(result);
        path = result;
    }
    ~Fixture() { std::error_code error; fs::remove_all(path, error); }
};

int main()
{
    Fixture fixture;
    const auto root = fixture.path / "photos";
    fs::create_directories(root / "Urlaub 2");
    fs::create_directories(root / "Urlaub 10");
    fs::create_directories(fixture.path / "outside");
    std::ofstream(root / "Urlaub 2" / "folder.jpg") << "test";
    std::ofstream(root / "Urlaub 2" / "Bäume.PNG") << "test";
    std::ofstream(root / "Bild 10.jpg") << "test";
    std::ofstream(root / "Bild 2.jpg") << "test";
    std::ofstream(root / "notes.txt") << "test";
    std::ofstream(fixture.path / "outside" / "private.jpg") << "test";
    fs::create_directory_symlink(fixture.path / "outside", root / "Escape");
    fs::create_symlink(fixture.path / "outside" / "private.jpg", root / "Escape.jpg");
    fs::create_directory_symlink(root / "Urlaub 2", root / "Alias");
    const Source source{"test", "Photos", root.string()};
    const auto listing = browseLocalDirectory(source, "");
    assert(listing.success && listing.entries.size() == 5);
    assert(listing.entries[0].name == "Alias");
    assert(listing.entries[0].cover == "Alias/folder.jpg");
    assert(listing.entries[1].name == "Urlaub 2");
    assert(listing.entries[1].cover == "Urlaub 2/folder.jpg");
    assert(listing.entries[2].name == "Urlaub 10");
    assert(listing.entries[2].cover.empty());
    assert(listing.entries[3].name == "Bild 2.jpg");
    assert(listing.entries[3].cover == "Bild 2.jpg");
    const auto nested = browseLocalDirectory(source, "Urlaub 2");
    assert(nested.success && nested.entries.size() == 2);
    assert(nested.entries[0].relativePath == "Urlaub 2/Bäume.PNG");
    const auto alias = browseLocalDirectory(source, "Alias");
    assert(alias.success && alias.entries.size() == 2);
    assert(alias.entries[0].relativePath == "Alias/Bäume.PNG");
    const auto empty = browseLocalDirectory(source, "Urlaub 10");
    assert(empty.success && empty.entries.empty());
    assert(!browseLocalDirectory(source, "Escape").success);
    assert(!browseLocalDirectory(source, "../outside").success);
    assert(!browseLocalDirectory(source, "/").success);
    assert(!browseLocalDirectory(source, "missing").success);
    const std::string png("\x89PNG\r\n\x1a\n", 8);
    std::ofstream(root / "Urlaub 2" / "Bäume.PNG", std::ios::binary) << png;
    const auto photo = readLocalPhoto(source, "Alias/Bäume.PNG", 1024);
    assert(photo.status == PhotoReadStatus::ready);
    assert(photo.contentType == "image/png" && photo.bytes == png);
    assert(readLocalPhoto(source, "Alias/Bäume.PNG", 7).status == PhotoReadStatus::exceedsBudget);
    assert(readLocalPhoto(source, "Bild 2.jpg", 1024).status == PhotoReadStatus::unsupported);
    assert(readLocalPhoto(source, "Escape.jpg", 1024).status == PhotoReadStatus::unavailable);
    assert(readLocalPhoto(source, "Escape/private.jpg", 1024).status == PhotoReadStatus::unavailable);
    assert(readLocalPhoto(source, "../outside/private.jpg", 1024).status == PhotoReadStatus::unavailable);
    assert(readLocalPhoto(source, (root / "Bild 2.jpg").string(), 1024).status == PhotoReadStatus::unavailable);
    assert(::mkfifo((root / "pipe.jpg").c_str(), 0600) == 0);
    assert(readLocalPhoto(source, "pipe.jpg", 1024).status == PhotoReadStatus::unavailable);
    // Browse a synthetic large collection without decoding its originals.
    // Check natural ordering across the whole directory, not just one page.
    const auto big = root / "Large collection";
    fs::create_directory(big);
    for (int i = 1000; i >= 1; --i)
        std::ofstream(big / ("Bild " + std::to_string(i) + ".jpg")) << "fixture";
    std::ofstream(big / "notes.txt") << "not a photo";
    const auto many = browseLocalDirectory(source,"Large collection");
    assert(many.success && many.entries.size() == 1000);
    for (std::size_t i = 0; i < many.entries.size(); ++i) {
        const auto name = "Bild " + std::to_string(i+1) + ".jpg";
        assert(many.entries[i].name == name);
        assert(many.entries[i].relativePath == "Large collection/" + name);
        assert(many.entries[i].cover == many.entries[i].relativePath);
    }
    std::cout << "IMAGE_GALLERY_LOCAL_LARGE_COLLECTION=PASS\n";
    std::cout << "IMAGE_GALLERY_LOCAL_BROWSE=PASS\n";
    std::cout << "IMAGE_GALLERY_LOCAL_READ=PASS\n";
}
