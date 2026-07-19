#include "suitebridge_tvscraper_adapter.h"

#include <vdr/epg.h>

#if defined(SUITEBRIDGE_TVSCRAPER_SERVICES_HEADER)
#include SUITEBRIDGE_TVSCRAPER_SERVICES_HEADER
#define SUITEBRIDGE_HAVE_TVSCRAPER_SERVICES 1
#elif __has_include(<vdr/plugins/tvscraper/services.h>)
#include <vdr/plugins/tvscraper/services.h>
#define SUITEBRIDGE_HAVE_TVSCRAPER_SERVICES 1
#elif __has_include(<tvscraper/services.h>)
#include <tvscraper/services.h>
#define SUITEBRIDGE_HAVE_TVSCRAPER_SERVICES 1
#else
#define SUITEBRIDGE_HAVE_TVSCRAPER_SERVICES 0
#endif

bool SuiteBridgeTvScraperAdapter::Available() noexcept
{
  return SUITEBRIDGE_HAVE_TVSCRAPER_SERVICES != 0;
}

SuiteBridgeArtworkReference
SuiteBridgeTvScraperAdapter::ResolvePreferredArtwork(
    const cEvent &event) const
{
#if SUITEBRIDGE_HAVE_TVSCRAPER_SERVICES
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
#else
  (void)event;
  return {};
#endif
}
