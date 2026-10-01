#pragma once

#include <cstdint>
#include <string>
#include <vector>

class Database;

enum class HumanAccountRepositoryStatus
{
    ok,
    invalid,
    notFound,
    revisionConflict,
    storageError,
};

struct HumanAccountRecord
{
    std::string accountId;
    std::string actorId;
    std::string displayName;
    bool active = false;
    std::uint64_t revision = 0;
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

    bool ensureAccountInActiveTransaction(
        const std::string& accountId,
        const std::string& actorId,
        const std::string& displayName);

    HumanAccountRepositoryStatus updateDisplayNameInActiveTransaction(
        const std::string& accountId,
        std::uint64_t expectedRevision,
        const std::string& displayName);

    HumanAccountRepositoryStatus setActiveInActiveTransaction(
        const std::string& accountId,
        std::uint64_t expectedRevision,
        bool active);

    HumanAccountLookupResult findByAccountId(
        const std::string& accountId) const;

    HumanAccountLookupResult findByActorId(
        const std::string& actorId) const;

    HumanAccountListResult listAll() const;

private:
    Database& database_;
};
