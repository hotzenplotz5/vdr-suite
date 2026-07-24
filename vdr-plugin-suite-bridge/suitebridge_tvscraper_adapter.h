#ifndef VDR_SUITE_BRIDGE_TVSCRAPER_ADAPTER_H
#define VDR_SUITE_BRIDGE_TVSCRAPER_ADAPTER_H

#include "suitebridge_artwork_reference.h"
#include "suitebridge_epg_metadata.h"

class cEvent;

class SuiteBridgeTvScraperAdapter final {
public:
  static bool Available() noexcept;

  SuiteBridgeEpgMediaType ResolveMediaType(
      const cEvent &event) const;

  SuiteBridgeArtworkReference ResolvePreferredArtwork(
      const cEvent &event) const;

  SuiteBridgeEpgMetadata ResolveMetadata(
      const cEvent &event) const;
};

#endif
