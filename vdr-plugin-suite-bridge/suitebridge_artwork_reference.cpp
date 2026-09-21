#include "suitebridge_artwork_reference.h"

bool SuiteBridgeArtworkReference::Valid() const noexcept
{
  return provider != SuiteBridgeArtworkProvider::None &&
      !path.empty() && width > 0 && height > 0;
}
