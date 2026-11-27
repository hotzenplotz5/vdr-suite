#include "ImageGalleryApi.h"
#include <cassert>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>

namespace fs = std::filesystem;
using namespace vdrsuite::imagegallery;

int main()
{
    auto pattern = (fs::temp_directory_path() / "suite-gallery-api-XXXXXX").string();
    const auto* path = ::mkdtemp(pattern.data());
    assert(path);
    const fs::path root(path);
    fs::create_directory(root / "photos");
    const std::string photo("\xff\xd8\xff",3);
    std::ofstream(root / "photos" / "Bild 2.jpg",std::ios::binary) << photo;
    const ImageGalleryApi api("default",ImageGallerySourceSettings(
        root / "sources.tsv",{{"pictures","Pictures",(root / "photos").string()}}),
        [](const std::string&,const std::string& bytes){return bytes;});
    auto get = [&](const std::string& endpoint, bool enabled = true) {
        return api.get("/api/v1/addons/image/" + endpoint,enabled);
    };
    assert(get("library?backendId=default",false).body.find("\"enabled\":false") != std::string::npos);
    assert(get("library?backendId=default").body.find(root.string()) == std::string::npos);
    assert(get("sources?backendId=default",false).body.find(root.string()) != std::string::npos);
    assert(get("browse?backendId=default&sourceId=pictures&path=").body.find("Bild 2.jpg") != std::string::npos);
    fs::create_directory(root / "photos" / "Urlaub");
    std::ofstream(root / "photos" / "Urlaub" / "Meer.jpg",std::ios::binary) << photo;
    fs::create_directory(root / "outside");
    std::ofstream(root / "outside" / "Secret.jpg",std::ios::binary) << photo;
    fs::create_directory_symlink(root / "outside",root / "photos" / "escape");
    auto sampled = get("library?backendId=default&sample=1");
    assert(sampled.statusCode == 200 && sampled.body.find("Urlaub/Meer.jpg") != std::string::npos);
    assert(sampled.body.find("Bild 2.jpg") != std::string::npos && sampled.body.find("Secret.jpg") == std::string::npos);
    assert(sampled.body.find(root.string()) == std::string::npos);
    assert(get("library?backendId=default&sample=1",false).body.find("\"sample\":[]") != std::string::npos);
    for (int i = 0; i < 20; ++i)
        std::ofstream(root / "photos" / ("Photo" + std::to_string(i) + ".jpg"),std::ios::binary) << photo;
    sampled = get("library?backendId=default&sample=1");
    std::size_t count = 0, offset = 0;
    while ((offset=sampled.body.find("\"sourceId\":",offset)) != std::string::npos) { ++count; ++offset; }
    assert(count == 9);
    assert(get("library?backendId=default&sample=2").statusCode == 400);
    assert(get("browse?backendId=default&sourceId=pictures&path=",false).statusCode == 404);
    const auto original = get("photo?backendId=default&sourceId=pictures&path=Bild%202.jpg");
    assert(original.statusCode == 200 && original.body == photo && original.contentType == "image/jpeg");
    assert(get("preview?backendId=default&sourceId=pictures&path=Bild%202.jpg").body == photo);
    assert(get("photo?backendId=default&sourceId=pictures&path=../sources.tsv").statusCode != 200);
    assert(get("library?backendId=other").statusCode == 404);
    assert(get("library?backendId=default&backendId=default").statusCode == 400);
    assert(api.save(R"({"backendId":"default","entries":""})").statusCode == 200);
    assert(get("library?backendId=default").body.find("\"sources\":[]") != std::string::npos);
    assert(get("browse?backendId=default&sourceId=pictures&path=").statusCode == 404);
    fs::remove_all(root);
    std::cout << "IMAGE_GALLERY_API=PASS\n";
}
