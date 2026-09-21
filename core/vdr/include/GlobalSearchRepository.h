#pragma once

#include "GlobalSearchResult.h"

#include <cstdint>
#include <string>

class Database;

class GlobalSearchRepository
{
public:
    explicit GlobalSearchRepository(Database& database);

    bool ensureSchema();
    bool ready() const;

    GlobalSearchResult search(
        const std::string& backendId,
        const std::string& query,
        std::int64_t epgFrom,
        std::int64_t epgUntil,
        int limit,
        int offset) const;

    static std::string normalizeBackendId(const std::string& backendId);
    static std::string foldText(const std::string& value);

private:
    Database& database_;

    bool registerFoldFunction() const;
    bool backfillEpgPeople() const;
    void searchRecordings(GlobalSearchResult& result) const;
    void searchEpg(GlobalSearchResult& result) const;
};
