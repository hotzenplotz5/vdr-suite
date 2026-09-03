#include "GenreIndexRepository.h"

#include "CanonicalGenreRegistry.h"
#include "Database.h"

#include <sqlite3.h>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <iomanip>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace
{
#define reconcileEpgBrowseClassificationLocked \
    reconcileEpgBrowseClassificationLockedV2 [[maybe_unused]]
#include "GenreIndexRepositoryHelpers.inc"
#undef reconcileEpgBrowseClassificationLocked

#define executeGenreSchema executeGenreSchemaV2
#include "GenreIndexRepositorySchema.inc"
#undef executeGenreSchema

#include "GenreIndexRepositoryLiveParity.inc"

#include "GenreIndexRepositoryStorage.inc"
#include "GenreIndexRepositorySynchronization.inc"
#include "GenreIndexRepositoryArtwork.inc"
#include "GenreIndexRepositoryQueries.inc"
