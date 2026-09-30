#include "Database.h"
#include "FirstAdminBootstrapIssuanceService.h"
#include "FirstAdminBootstrapRepository.h"
#include "HumanAccountRepository.h"
#include "SecurityIdentityRepository.h"
#include "SecurityPermissionGrantRepository.h"

#include <cerrno>
#include <cstdlib>
#include <iostream>
#include <string>
#include <unistd.h>

namespace
{
bool parseLifetime(const std::string& value, int& lifetimeSeconds)
{
    if (value.empty() || value.size() > 6)
    {
        return false;
    }

    char* end = nullptr;
    errno = 0;
    const long parsed = std::strtol(
        value.c_str(),
        &end,
        10);
    if (errno != 0 ||
        end == nullptr ||
        *end != '\0' ||
        parsed <
            FirstAdminBootstrapIssuanceService::MinimumLifetimeSeconds ||
        parsed >
            FirstAdminBootstrapIssuanceService::MaximumLifetimeSeconds)
    {
        return false;
    }

    lifetimeSeconds = static_cast<int>(parsed);
    return true;
}

void printUsage()
{
    std::cerr
        << "usage: vdr-suite-first-admin-bootstrap "
           "[--database PATH] [--ttl-seconds 300..3600]"
        << std::endl;
}

int exitForStatus(FirstAdminBootstrapIssuanceStatus status)
{
    switch (status)
    {
        case FirstAdminBootstrapIssuanceStatus::invalidLifetime:
            return 64;
        case FirstAdminBootstrapIssuanceStatus::claimed:
            return 77;
        case FirstAdminBootstrapIssuanceStatus::conflict:
            return 75;
        case FirstAdminBootstrapIssuanceStatus::entropyUnavailable:
            return 74;
        case FirstAdminBootstrapIssuanceStatus::storageError:
            return 74;
        case FirstAdminBootstrapIssuanceStatus::issued:
            return 0;
        case FirstAdminBootstrapIssuanceStatus::hashingUnavailable:
            return 74;
    }
    return 74;
}

const char* messageForStatus(
    FirstAdminBootstrapIssuanceStatus status)
{
    switch (status)
    {
        case FirstAdminBootstrapIssuanceStatus::claimed:
            return "server is already claimed";
        case FirstAdminBootstrapIssuanceStatus::conflict:
            return "an active first-admin bootstrap already exists";
        case FirstAdminBootstrapIssuanceStatus::invalidLifetime:
            return "invalid bootstrap lifetime";
        case FirstAdminBootstrapIssuanceStatus::entropyUnavailable:
            return "secure entropy is unavailable";
        case FirstAdminBootstrapIssuanceStatus::hashingUnavailable:
            return "bootstrap verifier hashing failed";
        case FirstAdminBootstrapIssuanceStatus::storageError:
            return "bootstrap persistence failed";
        case FirstAdminBootstrapIssuanceStatus::issued:
            return "";
    }
    return "bootstrap issuance failed";
}
}

int main(int argc, char** argv)
{
    if (geteuid() != 0)
    {
        std::cerr
            << "vdr-suite-first-admin-bootstrap must be run by root"
            << std::endl;
        return 77;
    }

    std::string databasePath =
        "/var/lib/vdr-suite/vdr-suite.db";
    int lifetimeSeconds =
        FirstAdminBootstrapIssuanceService::DefaultLifetimeSeconds;

    for (int index = 1; index < argc; ++index)
    {
        const std::string argument = argv[index];
        if (argument == "--database" && index + 1 < argc)
        {
            databasePath = argv[++index];
        }
        else if (argument == "--ttl-seconds" &&
                 index + 1 < argc)
        {
            if (!parseLifetime(
                    argv[++index],
                    lifetimeSeconds))
            {
                std::cerr
                    << "invalid bootstrap TTL"
                    << std::endl;
                printUsage();
                return 64;
            }
        }
        else
        {
            printUsage();
            return 64;
        }
    }

    if (databasePath.empty())
    {
        std::cerr << "invalid bootstrap database path" << std::endl;
        return 64;
    }

    Database database;
    if (!database.open(databasePath))
    {
        std::cerr
            << "failed to open VDR-Suite security database"
            << std::endl;
        return 74;
    }

    SecurityIdentityRepository identities(database);
    HumanAccountRepository accounts(database);
    SecurityPermissionGrantRepository grants(database);
    FirstAdminBootstrapRepository bootstraps(database);

    if (!identities.ensureSchema() ||
        !accounts.ensureSchema() ||
        !grants.ensureSchema() ||
        !bootstraps.ensureSchema())
    {
        std::cerr
            << "failed to initialize first-admin bootstrap repositories"
            << std::endl;
        return 74;
    }

    FirstAdminBootstrapIssuanceService issuer(bootstraps);
    FirstAdminBootstrapIssuanceResult result =
        issuer.issue(lifetimeSeconds);

    if (result.status !=
            FirstAdminBootstrapIssuanceStatus::issued ||
        !result.bootstrap.has_value())
    {
        std::cerr
            << "first-admin bootstrap not issued: "
            << messageForStatus(result.status)
            << std::endl;
        return exitForStatus(result.status);
    }

    IssuedFirstAdminBootstrap& issued =
        *result.bootstrap;

    std::cout
        << "bootstrap_id=" << issued.bootstrapId << '\n'
        << "setup_secret=" << issued.setupSecret << '\n'
        << "expires_at=" << issued.expiresAt << std::endl;

    issued.clearSecret();
    return 0;
}
