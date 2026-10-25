#include "AccountabilityEventRepository.h"
#include "CredentialVerifierRepository.h"
#include "Database.h"
#include "FirstAdminBootstrapIssuanceService.h"
#include "FirstAdminBootstrapRepository.h"
#include "FirstAdminClaimService.h"
#include "HumanAccountRepository.h"
#include "SecurityIdentityProvisioningRepository.h"
#include "SecurityIdentityRepository.h"
#include "SecurityPermissionGrantRepository.h"

#include <chrono>
#include <cstddef>
#include <iostream>
#include <string>
#include <termios.h>
#include <unistd.h>
#include <utility>

namespace
{
void secureWipe(std::string& value) noexcept
{
    volatile char* bytes = value.empty()
        ? nullptr
        : const_cast<volatile char*>(value.data());
    for (std::size_t i = 0; i < value.size(); ++i)
        bytes[i] = 0;
    value.clear();
}

bool readText(const char* label, std::string& value, std::size_t maxSize)
{
    std::cout << label << std::flush;
    return static_cast<bool>(std::getline(std::cin, value)) &&
        !value.empty() && value.size() <= maxSize;
}

bool readPasswords(std::string& password)
{
    termios original{};
    if (tcgetattr(STDIN_FILENO, &original) != 0) return false;
    termios hidden = original;
    hidden.c_lflag &= static_cast<tcflag_t>(~ECHO);
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &hidden) != 0) return false;

    std::string confirmation;
    std::cout << "New admin password: " << std::flush;
    const bool first = static_cast<bool>(std::getline(std::cin, password));
    std::cout << "\nConfirm password: " << std::flush;
    const bool second = static_cast<bool>(std::getline(std::cin, confirmation));
    const int restored = tcsetattr(STDIN_FILENO, TCSAFLUSH, &original);
    std::cout << '\n' << std::flush;

    const bool valid = first && second && restored == 0 &&
        password.size() >= 12 && password.size() <= 1024 &&
        password == confirmation;
    secureWipe(confirmation);
    if (!valid) secureWipe(password);
    return valid;
}

const char* issuerError(FirstAdminBootstrapIssuanceStatus status)
{
    switch (status)
    {
        case FirstAdminBootstrapIssuanceStatus::claimed:
            return "server already has an administrator";
        case FirstAdminBootstrapIssuanceStatus::conflict:
            return "another active setup authorization exists; let it expire before retrying";
        case FirstAdminBootstrapIssuanceStatus::entropyUnavailable:
            return "secure random source unavailable";
        case FirstAdminBootstrapIssuanceStatus::hashingUnavailable:
            return "setup authorization hashing failed";
        case FirstAdminBootstrapIssuanceStatus::invalidLifetime:
            return "invalid setup authorization lifetime";
        case FirstAdminBootstrapIssuanceStatus::storageError:
            return "setup authorization persistence failed";
        case FirstAdminBootstrapIssuanceStatus::issued:
            return "";
    }
    return "setup authorization failed";
}

const char* claimError(FirstAdminClaimStatus status)
{
    switch (status)
    {
        case FirstAdminClaimStatus::invalidRequest:
            return "invalid account fields";
        case FirstAdminClaimStatus::claimed:
            return "server already has an administrator";
        case FirstAdminClaimStatus::conflict:
            return "account or credential conflicts with existing data";
        case FirstAdminClaimStatus::bootstrapExpired:
        case FirstAdminClaimStatus::bootstrapConsumed:
        case FirstAdminClaimStatus::bootstrapInvalidated:
        case FirstAdminClaimStatus::bootstrapNotFound:
        case FirstAdminClaimStatus::bootstrapRejected:
            return "internal setup authorization was rejected";
        case FirstAdminClaimStatus::entropyUnavailable:
            return "secure random source unavailable";
        case FirstAdminClaimStatus::hashingUnavailable:
            return "password hashing unavailable";
        case FirstAdminClaimStatus::storageError:
            return "database transaction failed";
        case FirstAdminClaimStatus::success:
            return "";
    }
    return "first administrator setup failed";
}

std::string localRequestId()
{
    const auto ticks = std::chrono::system_clock::now().time_since_epoch().count();
    return "local-first-admin-setup-" + std::to_string(getpid()) +
        "-" + std::to_string(static_cast<long long>(ticks));
}
}

int main(int argc, char** argv)
{
    if (geteuid() != 0)
    {
        std::cerr << "vdr-suite-first-admin-setup requires root" << std::endl;
        return 77;
    }
    if (!isatty(STDIN_FILENO) || !isatty(STDOUT_FILENO))
    {
        std::cerr << "an interactive local terminal is required" << std::endl;
        return 77;
    }

    std::string databasePath = "/var/lib/vdr-suite/vdr-suite.db";
    if (argc == 3 && std::string(argv[1]) == "--database")
        databasePath = argv[2];
    else if (argc != 1)
    {
        std::cerr << "usage: vdr-suite-first-admin-setup [--database PATH]" << std::endl;
        return 64;
    }
    if (databasePath.empty() || databasePath.front() != '/')
    {
        std::cerr << "database path must be absolute" << std::endl;
        return 64;
    }

    Database database;
    if (!database.open(databasePath))
    {
        std::cerr << "cannot open the VDR-Suite database" << std::endl;
        return 74;
    }
    SecurityIdentityRepository identities(database);
    SecurityIdentityProvisioningRepository provisioning(database);
    HumanAccountRepository accounts(database);
    CredentialVerifierRepository verifiers(database);
    SecurityPermissionGrantRepository grants(database);
    AccountabilityEventRepository accountability(database);
    FirstAdminBootstrapRepository bootstraps(database);
    if (!identities.ensureSchema() || !accounts.ensureSchema() ||
        !verifiers.ensureSchema() || !grants.ensureSchema() ||
        !accountability.ensureSchema() || !bootstraps.ensureSchema())
    {
        std::cerr << "cannot initialize first-admin security repositories" << std::endl;
        return 74;
    }
    const FirstAdminClaimState state = bootstraps.claimState();
    if (state != FirstAdminClaimState::unclaimed)
    {
        std::cerr << (state == FirstAdminClaimState::claimed
            ? "server already has an administrator; use account recovery if needed"
            : "cannot determine first-admin claim state") << std::endl;
        return 77;
    }

    std::string login;
    std::string displayName;
    std::string password;
    if (!readText("Admin login name: ", login, 128) ||
        !readText("Display name: ", displayName, 256) ||
        !readPasswords(password))
    {
        secureWipe(password);
        std::cerr << "setup aborted: missing/invalid input or passwords do not match (minimum 12 characters)" << std::endl;
        return 64;
    }

    FirstAdminBootstrapIssuanceService issuer(bootstraps);
    FirstAdminBootstrapIssuanceResult issued = issuer.issue();
    if (issued.status != FirstAdminBootstrapIssuanceStatus::issued ||
        !issued.bootstrap.has_value())
    {
        secureWipe(password);
        std::cerr << "setup aborted: " << issuerError(issued.status) << std::endl;
        return 74;
    }

    FirstAdminClaimRequest request;
    request.bootstrapId = std::move(issued.bootstrap->bootstrapId);
    request.setupSecret = std::move(issued.bootstrap->setupSecret);
    issued.bootstrap->clearSecret();
    request.loginName = std::move(login);
    request.displayName = std::move(displayName);
    request.password = std::move(password);
    secureWipe(password);
    request.requestId = localRequestId();

    FirstAdminClaimService claimer(
        database, bootstraps, provisioning, accounts, verifiers,
        grants, accountability);
    const FirstAdminClaimResult result = claimer.claim(std::move(request));
    if (result.status != FirstAdminClaimStatus::success)
    {
        std::cerr << "setup failed: " << claimError(result.status) << std::endl;
        return 74;
    }

    std::cout << "First administrator created successfully.\n"
                 "Sign in to VDR-Suite using the chosen login and password."
              << std::endl;
    return 0;
}
