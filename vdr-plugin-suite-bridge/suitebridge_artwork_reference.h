#ifndef VDR_SUITE_BRIDGE_ARTWORK_REFERENCE_H
#define VDR_SUITE_BRIDGE_ARTWORK_REFERENCE_H

#include <string>

enum class SuiteBridgeArtworkProvider {
  None,
  TvScraper,
};

struct SuiteBridgeArtworkReference final {
  SuiteBridgeArtworkProvider provider = SuiteBridgeArtworkProvider::None;
  std::string path;
  int width = 0;
  int height = 0;

  bool Valid() const noexcept;
};

#endif
