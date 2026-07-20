#include "suitebridge_tvscraper_adapter.h"

#include "suitebridge_image_dimensions.h"
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

  SuiteBridgeImageDimensions actualDimensions;
  if (!SuiteBridgeReadImageDimensions(media.path, actualDimensions)) {
    isyslog(
        "suitebridge: tvscraper result=invalid-artwork-file event=%u type=%d path=%s reported_width=%d reported_height=%d",
        event.EventID(),
        static_cast<int>(request.m_scraperVideo->getVideoType()),
        media.path.c_str(),
        media.width,
        media.height);
    return {};
  }

  SuiteBridgeArtworkReference reference;
  reference.provider = SuiteBridgeArtworkProvider::TvScraper;
  reference.path = media.path;
  reference.width = actualDimensions.width;
  reference.height = actualDimensions.height;

  isyslog(
      "suitebridge: tvscraper result=artwork event=%u type=%d path=%s reported_width=%d reported_height=%d actual_width=%d actual_height=%d",
      event.EventID(),
      static_cast<int>(request.m_scraperVideo->getVideoType()),
      media.path.c_str(),
      media.width,
      media.height,
      actualDimensions.width,
      actualDimensions.height);

  return reference;
}
