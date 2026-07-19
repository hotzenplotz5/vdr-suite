#include "suitebridge_tvscraper_adapter.h"

#include "services.h"

#include <vdr/epg.h>
#include <vdr/tools.h>

bool SuiteBridgeTvScraperAdapter::Available() noexcept
{
  return true;
}

SuiteBridgeArtworkReference
SuiteBridgeTvScraperAdapter::ResolvePreferredArtwork(
    const cEvent &event) const
{
  cGetScraperVideo request(&event, nullptr);
  cPlugin *scraper = request.call();

  if (!scraper) {
    isyslog(
        "suitebridge: tvscraper result=service-unavailable event=%u title=%s",
        event.EventID(),
        event.Title() ? event.Title() : "");
    return {};
  }

  if (!request.m_scraperVideo) {
    isyslog(
        "suitebridge: tvscraper result=no-video event=%u title=%s",
        event.EventID(),
        event.Title() ? event.Title() : "");
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

  isyslog(
      "suitebridge: tvscraper result=%s event=%u type=%d path=%s width=%d height=%d",
      reference.Valid() ? "artwork" : "no-artwork",
      event.EventID(),
      static_cast<int>(request.m_scraperVideo->getVideoType()),
      media.path.c_str(),
      media.width,
      media.height);

  return reference.Valid() ? reference : SuiteBridgeArtworkReference{};
}
