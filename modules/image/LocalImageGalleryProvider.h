#pragma once

#include "ImageGalleryCatalog.h"

#include <cstddef>

namespace vdrsuite::imagegallery {

struct BrowseResult {
    bool success = false;
    std::vector<Entry> entries;
};

// The caller resolves the source ID to an authorized configured Source.
// Read just the requested directory; no recursive scan or image decoding.
BrowseResult browseLocalDirectory(const Source& source,
                                  const std::string& relativeDirectory);

// The same source-relative resolution is used for browse and image reads.
// The caller owns authorization and selects a byte budget for its use case.
enum class PhotoReadStatus { unavailable, unsupported, exceedsBudget, ready };
struct PhotoReadResult {
    PhotoReadStatus status = PhotoReadStatus::unavailable;
    std::string contentType;
    std::string bytes;
};

PhotoReadResult readLocalPhoto(const Source& source,
                               const std::string& relativePath,
                               std::size_t byteBudget);

} // namespace vdrsuite::imagegallery
