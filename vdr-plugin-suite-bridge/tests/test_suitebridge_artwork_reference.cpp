#include "suitebridge_artwork_reference.h"

#include <cassert>

int main()
{
  SuiteBridgeArtworkReference empty;
  assert(!empty.Valid());

  SuiteBridgeArtworkReference missingDimensions;
  missingDimensions.provider = SuiteBridgeArtworkProvider::TvScraper;
  missingDimensions.path = "/var/cache/vdr/plugins/tvscraper/example.jpg";
  assert(!missingDimensions.Valid());

  SuiteBridgeArtworkReference valid;
  valid.provider = SuiteBridgeArtworkProvider::TvScraper;
  valid.path = "/var/cache/vdr/plugins/tvscraper/example.jpg";
  valid.width = 1280;
  valid.height = 720;
  assert(valid.Valid());

  return 0;
}
