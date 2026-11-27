#pragma once

#include "ImageGalleryCatalog.h"

namespace vdrsuite::imagegallery {

struct BrowseResult {
    bool success = false;
    std::vector<Entry> entries;
};

// The caller resolves the source ID to an authorized configured Source.
// Read just the requested directory; no recursive scan or image decoding.
BrowseResult browseLocalDirectory(const Source& source,
                                  const std::string& relativeDirectory);

} // namespace vdrsuite::imagegallery
