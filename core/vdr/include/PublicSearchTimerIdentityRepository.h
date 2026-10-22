#pragma once

#include <optional>
#include <string>
#include <vector>

class Database;
class SearchTimer;

// Public IDs are lifecycle identities, NOT aliases for epgsearch's reusable
// native numeric IDs. Only a complete, successful provider catalogue may
// retire bindings. Each retired public ID remains permanently reserved.
class PublicSearchTimerIdentityRepository final
{
public:
    explicit PublicSearchTimerIdentityRepository(Database& database);
    bool ensureSchema();

    // All observations must belong to backendId and comprise one authoritative
    // unfiltered snapshot. The returned IDs align with observations in order.
    // An empty successful snapshot retires all active IDs for that backend.
    // On any validation/SQL failure nothing is committed.
    std::optional<std::vector<std::string>> reconcileAuthoritativeSnapshot(
        const std::string& backendId,
        const std::vector<SearchTimer>& observations);

private:
    Database& database_;
};
