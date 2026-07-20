#ifndef VDR_SUITE_BRIDGE_TVSCRAPER_ADAPTER_H
#define VDR_SUITE_BRIDGE_TVSCRAPER_ADAPTER_H

#include "suitebridge_artwork_reference.h"

class cEvent;

class SuiteBridgeTvScraperAdapter final {
public:
  static bool Available() noexcept;

  SuiteBridgeArtworkReference ResolvePreferredArtwork(
      const cEvent &event) const;
};

#endif
