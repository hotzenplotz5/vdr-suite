#pragma once

#include "ImageGalleryCatalog.h"

#include <filesystem>

namespace vdrsuite::imagegallery {

// Owned by the daemon, not chosen by HTTP requests. The distribution
// selects the first-run defaults. An explicitly saved empty list stays empty.
enum class SourceSettingsState { defaults, configured, unavailable };

struct SourceSettingsSnapshot {
    SourceSettingsState state = SourceSettingsState::unavailable;
    std::vector<Source> sources;
};

class ImageGallerySourceSettings {
public:
    ImageGallerySourceSettings(std::filesystem::path file,
                               std::vector<Source> defaults);
    SourceSettingsSnapshot load() const;
    bool save(const std::vector<Source>& sources) const;

private:
    std::filesystem::path file_;
    std::vector<Source> defaults_;
};

} // namespace vdrsuite::imagegallery
