#include "suitebridge_tvscraper_adapter.h"

#include "services.h"

#include <vdr/epg.h>

bool SuiteBridgeTvScraperAdapter::Available() noexcept
{
  return true;
}

SuiteBridgeArtworkReference
SuiteBridgeTvScraperAdapter::ResolvePreferredArtwork(
    const cEvent &event) const
{
  cGetScraperVideo request(&event, nullptr);
  if (!request.call() || !request.m_scraperVideo) {
    return {};
  }

  const cTvMedia media = request.m_scraperVideo->getImage(
      cImageLevels(
          eImageLevel::episodeMovie,
          eImageLevel::seasonMovie,
          eImageLevel::tvShowCollection,
          eImageLevel::anySeasonCollection),
      cOrientations(
          eOrientation::landscape,
          eOrientation::banner,
          eOrientation::portrait),
      true);

  SuiteBridgeArtworkReference reference;
  reference.provider = SuiteBridgeArtworkProvider::TvScraper;
  reference.path = media.path;
  reference.width = media.width;
  reference.height = media.height;
  return reference.Valid() ? reference : SuiteBridgeArtworkReference{};
}
