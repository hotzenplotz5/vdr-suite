#pragma once

#include <map>
#include <string>
#include <vector>

class Database;

// Existing Suite database; advisory codec preview cache only.
// Never authorizes an import or changes the VDR recording inventory.
namespace media_import_preview_store {
bool ensureSchema(Database& db);
bool load(Database& db, const std::string& backend,
          const std::string& sourceId, std::string& payload);
void save(Database& db, const std::string& backend,
          const std::string& sourceId, const std::string& payload);
void prune(Database& db, const std::string& backend,
           const std::vector<std::string>& sortedObservedIds,
           const std::map<std::string, std::string>& statuses, bool partial);
} // namespace media_import_preview_store
