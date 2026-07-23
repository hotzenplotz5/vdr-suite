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
#include <utility>
#include <vector>

namespace
{
#include "GenreIndexRepositoryHelpers.inc"
#include "GenreIndexRepositorySchema.inc"

#include "GenreIndexRepositoryStorage.inc"
#include "GenreIndexRepositorySynchronization.inc"
#include "GenreIndexRepositoryQueries.inc"
