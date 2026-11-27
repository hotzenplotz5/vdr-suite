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
