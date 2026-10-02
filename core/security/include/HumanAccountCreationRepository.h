#pragma once

#include <optional>
#include <string>

class Database;

struct HumanAccountCreateIdempotencyRecord
{
    std::string actorId;
    std::string idempotencyKey;
    std::string loginName;
    std::string displayName;
    std::string accountId;
};

class HumanAccountCreationRepository
{
public:
    explicit HumanAccountCreationRepository(Database& database);

    bool ensureSchema();

    std::optional<HumanAccountCreateIdempotencyRecord> find(
        const std::string& actorId,
        const std::string& idempotencyKey) const;

    bool insertInActiveTransaction(
        const HumanAccountCreateIdempotencyRecord& record);

private:
    Database& database_;
};
