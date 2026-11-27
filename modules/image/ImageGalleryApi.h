#pragma once

#include "DashboardController.h"
#include "ImageGallerySourceSettings.h"
#include <functional>

namespace vdrsuite::imagegallery {

// The authenticated HTTP owner supplies the backend and current activation
// state. This facade does not grant permissions or resolve a remote host.
class ImageGalleryApi {
public:
    using Preview = std::function<std::string(const std::string&,const std::string&)>;
    ImageGalleryApi(std::string localBackend, ImageGallerySourceSettings settings, Preview preview = {});
    ApiResponse get(const std::string& target, bool enabled) const;
    ApiResponse save(const std::string& body) const;

private:
    std::string localBackend_;
    ImageGallerySourceSettings settings_;
    Preview preview_;
};

} // namespace vdrsuite::imagegallery
