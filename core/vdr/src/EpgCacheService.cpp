#include "EpgCacheService.h"

EpgCacheService::EpgCacheService(
    EpgEventRepository& repository,
    VdrService& vdrService)
    : repository_(repository),
      vdrService_(vdrService)
{
}
