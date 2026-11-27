#pragma once

#include "DashboardController.h"
#include "ImageGallerySourceSettings.h"

namespace vdrsuite::imagegallery {

// The authenticated HTTP owner supplies the backend and current activation
// state. This facade does not grant permissions or resolve a remote host.
class ImageGalleryApi {
public:
    ImageGalleryApi(std::string localBackend, ImageGallerySourceSettings settings);
    ApiResponse get(const std::string& target, bool enabled) const;
    ApiResponse save(const std::string& body) const;

private:
    std::string localBackend_;
    ImageGallerySourceSettings settings_;
};

} // namespace vdrsuite::imagegallery
