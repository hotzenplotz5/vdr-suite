#pragma once

#include <string>
#include <vector>

namespace vdrsuite::imagegallery {

// Backend-owned configured sources; never construct these from a browse request.
struct Source {
    std::string id;
    std::string title;
    std::string root;
};

// Distribution default only, not an implicit source when settings already exist.
Source yavdrDefaultSource();

// The browse projection exposes no host-absolute filenames.
struct Entry {
    std::string name;
    std::string relativePath;
    bool directory = false;
};

// Provider supplies directory entries after its existing source-root checks.
// No file I/O, decoding, caching or authorization is performed here.
std::vector<Entry> galleryEntries(std::vector<Entry> entries);
bool isPhotoFilename(const std::string& name);

} // namespace vdrsuite::imagegallery
