#include "ImageGalleryCatalog.h"

#include <cassert>
#include <iostream>
#include <vector>

using namespace vdrsuite::imagegallery;

int main()
{
    const auto source = yavdrDefaultSource();
    assert(source.root == "/srv/picture");
    assert(!source.id.empty());
    assert(isPhotoFilename("Urlaub 01.JPEG"));
    assert(isPhotoFilename("Bäume.png"));
    assert(isPhotoFilename("photo.webp"));
    assert(!isPhotoFilename("bild.jpg.txt"));
    assert(!isPhotoFilename("index"));

    const auto entries = galleryEntries({
        {"Bild 10.jpg", "2026/Bild 10.jpg", false},
        {"Ordner 10", "Ordner 10", true},
        {"Bild 2.jpg", "2026/Bild 2.jpg", false},
        {"Ordner 2", "Ordner 2", true},
        {"notiz.txt", "notiz.txt", false},
        {"Bild 02.jpg", "2026/Bild 02.jpg", false},
        {"Bild 999999999999999999999999999999.jpg", "big.jpg", false},
        {"Bäume.png", "2026/Bäume.png", false}
    });
    assert(entries.size() == 7);
    assert(entries[0].name == "Ordner 2");
    assert(entries[1].name == "Ordner 10");
    assert(entries[2].name == "Bild 2.jpg");
    assert(entries[2].relativePath == "2026/Bild 2.jpg");
    assert(entries[3].name == "Bild 02.jpg");
    assert(entries[4].name == "Bild 10.jpg");
    assert(entries[5].name == "Bild 999999999999999999999999999999.jpg");
    assert(entries[6].name == "Bäume.png");
    assert(galleryEntries({}).empty());
    std::cout << "IMAGE_GALLERY_CATALOG=PASS\n";
}
