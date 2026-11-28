#include "ImageGallerySourceSettings.h"

#include <cassert>
#include <cstdlib>
#include <fstream>
#include <iostream>

namespace fs = std::filesystem;
using namespace vdrsuite::imagegallery;

struct Fixture {
    fs::path path;
    Fixture() {
        auto pattern = (fs::temp_directory_path() / "suite-image-settings-XXXXXX").string();
        const auto* result = ::mkdtemp(pattern.data());
        assert(result);
        path = result;
    }
    ~Fixture() { std::error_code error; fs::remove_all(path, error); }
};

int main()
{
    Fixture fixture;
    const auto file = fixture.path / "image.tsv";
    const std::vector<Source> defaults{yavdrDefaultSource()};
    const ImageGallerySourceSettings settings(file, defaults);
    assert(settings.load().state == SourceSettingsState::defaults);
    assert(settings.load().sources[0].root == "/srv/picture");
    assert(!fs::exists(file));
    const auto offline = fixture.path / "NAS currently offline";
    const std::vector<Source> entries{
        {"pictures", "Meine Bilder", "/srv/picture"},
        {"nas", "Urlaub & Bäume", offline.string()}
    };
    assert(settings.save(entries));
    const ImageGallerySourceSettings reopened(file, {});
    const auto saved = reopened.load();
    assert(saved.state == SourceSettingsState::configured);
    assert(saved.sources.size() == 2);
    assert(saved.sources[1].id == "nas");
    assert(saved.sources[1].title == "Urlaub & Bäume");
    assert(saved.sources[1].root == offline.string());
    assert(!fs::exists(offline));
    assert(!settings.save({{"x", "Relative", "not/absolute"}}));
    assert(!settings.save({{"x", "Whole host", "/"}}));
    assert(!settings.save({entries[0], entries[0]}));
    assert(settings.load().sources.size() == 2);
    assert(settings.save({}));
    assert(settings.load().state == SourceSettingsState::configured);
    assert(settings.load().sources.empty());
    std::ofstream(file) << "broken\n";
    assert(settings.load().state == SourceSettingsState::unavailable);
    fs::remove(file);
    const auto unrelated = fixture.path / "unrelated";
    std::ofstream(unrelated) << "untouched";
    fs::create_symlink(unrelated, file);
    assert(settings.load().state == SourceSettingsState::unavailable);
    assert(!settings.save(entries));
    std::ifstream original(unrelated);
    std::string value;
    original >> value;
    assert(value == "untouched");
    std::cout << "IMAGE_GALLERY_SOURCE_SETTINGS=PASS\n";
}
