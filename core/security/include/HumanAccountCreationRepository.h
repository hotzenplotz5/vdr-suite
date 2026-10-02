#pragma once

#include <string>

class Database;

enum class HumanAccountCreateIdempotencyStatus
{
    ok,
    notFound,
    invalid,
    conflict,
    storageError,
};

struct HumanAccountCreateIdempotencyRecord
{
    std::string requestActorId;
    std::string idempotencyKey;
    std::string loginName;
    std::string displayName;
    std::string accountId;
};

struct HumanAccountCreateIdempotencyLookupResult
{
    HumanAccountCreateIdempotencyStatus status =
        HumanAccountCreateIdempotencyStatus::storageError;
    HumanAccountCreateIdempotencyRecord record;
};

class HumanAccountCreationRepository
{
public:
    explicit HumanAccountCreationRepository(Database& database);

    bool ensureSchema();

    HumanAccountCreateIdempotencyLookupResult find(
        const std::string& requestActorId,
        const std::string& idempotencyKey) const;

    HumanAccountCreateIdempotencyStatus recordInActiveTransaction(
        const HumanAccountCreateIdempotencyRecord& record);

private:
    Database& database_;
};
