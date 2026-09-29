#pragma once

#include <string>
#include <vector>

class Database;

enum class HumanAccountRepositoryStatus
{
    ok,
    invalid,
    notFound,
    storageError,
};

struct HumanAccountRecord
{
    std::string accountId;
    std::string actorId;
    std::string displayName;
    bool active = false;
};

struct HumanAccountLookupResult
{
    HumanAccountRepositoryStatus status =
        HumanAccountRepositoryStatus::storageError;
    HumanAccountRecord account;
};

struct HumanAccountListResult
{
    HumanAccountRepositoryStatus status =
        HumanAccountRepositoryStatus::storageError;
    std::vector<HumanAccountRecord> accounts;
};

class HumanAccountRepository
{
public:
    explicit HumanAccountRepository(Database& database);

    bool ensureSchema();

    HumanAccountLookupResult findByAccountId(
        const std::string& accountId) const;

    HumanAccountListResult listAll() const;

private:
    Database& database_;
};
