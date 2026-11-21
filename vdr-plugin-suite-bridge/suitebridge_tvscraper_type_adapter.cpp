#include "suitebridge_tvscraper_adapter.h"

#include "services.h"

#include <vdr/epg.h>
#include <vdr/tools.h>

SuiteBridgeEpgMediaType SuiteBridgeTvScraperAdapter::ResolveMediaType(
    const cEvent &event) const
{
  cGetScraperVideo request(&event, nullptr);
  cPlugin *scraper = request.call();
  if (!scraper || !request.m_scraperVideo) {
    return SuiteBridgeEpgMediaType::None;
  }

  switch (request.m_scraperVideo->getVideoType()) {
  case tSeries:
    return SuiteBridgeEpgMediaType::Series;
  case tMovie:
    return SuiteBridgeEpgMediaType::Movie;
  case tNone:
    break;
  }

  return SuiteBridgeEpgMediaType::None;
}
