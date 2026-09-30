#pragma once

#include <string>

class Database;

enum class FirstAdminClaimState
{
    unavailable,
    unclaimed,
    claimed,
};

enum class FirstAdminBootstrapStatus
{
    ok,
    invalid,
    notFound,
    storageError,
    claimed,
    conflict,
    expired,
    consumed,
    invalidated,
    transactionRequired,
};

struct FirstAdminBootstrapRegistration
{
    std::string bootstrapId;
    std::string verifierHash;
    std::string expiresAt;
};

struct StoredFirstAdminBootstrap
{
    std::string bootstrapId;
    std::string verifierHash;
    std::string expiresAt;
    std::string consumedAt;
    std::string invalidatedAt;
    bool expired = false;
    bool consumed = false;
    bool invalidated = false;
};

struct FirstAdminBootstrapLookupResult
{
    FirstAdminBootstrapStatus status =
        FirstAdminBootstrapStatus::storageError;
    StoredFirstAdminBootstrap bootstrap;
};

class FirstAdminBootstrapRepository
{
public:
    explicit FirstAdminBootstrapRepository(Database& database);

    bool ensureSchema();

    FirstAdminClaimState claimState() const;

    FirstAdminBootstrapStatus registerBootstrap(
        const FirstAdminBootstrapRegistration& registration);

    FirstAdminBootstrapLookupResult findById(
        const std::string& bootstrapId) const;

    FirstAdminBootstrapStatus consumeInActiveTransaction(
        const std::string& bootstrapId);

    FirstAdminBootstrapStatus invalidateInActiveTransaction(
        const std::string& bootstrapId);

    static bool supportsVerifierHash(const std::string& verifierHash);

private:
    Database& database_;
};
