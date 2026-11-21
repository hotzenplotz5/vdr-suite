#ifndef VDR_SUITE_BRIDGE_IMAGE_DIMENSIONS_H
#define VDR_SUITE_BRIDGE_IMAGE_DIMENSIONS_H

#include <string>

struct SuiteBridgeImageDimensions final {
  int width = 0;
  int height = 0;

  bool Valid() const noexcept
  {
    return width > 0 && height > 0;
  }
};

bool SuiteBridgeReadImageDimensions(
    const std::string &path,
    SuiteBridgeImageDimensions &dimensions) noexcept;

#endif
