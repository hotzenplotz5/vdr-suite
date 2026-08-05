#pragma once

#include "GlobalSearchResult.h"

#include <string>
#include <vector>

class Database;

class GlobalSearchPersonPortraitRepository
{
public:
    explicit GlobalSearchPersonPortraitRepository(Database& database);

    std::vector<GlobalSearchPersonPortrait> findForBackend(
        const std::string& backendId) const;

private:
    Database& database_;
};