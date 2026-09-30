#include "AccountabilityEventRepository.h"
#include "BrowserSessionCredentialRepository.h"
#include "CredentialVerifierRepository.h"
#include "Database.h"
#include "HumanAccountRecoveryService.h"
#include "HumanAccountRepository.h"
#include "SecurityIdentityRepository.h"

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
    for (std::size_t index = 0; index < value.size(); ++index)
    {
        bytes[index] = 0;
    }
    value.clear();
}

void printUsage()
{
    std::cerr
        << "usage: vdr-suite-human-account-recover "
           "--account-id ID --login LOGIN [--database PATH]"
        << std::endl;
}

bool readPassword(std::string& password)
{
    password.clear();
    if (!isatty(STDIN_FILENO))
    {
        return static_cast<bool>(
                   std::getline(std::cin, password)) &&
            !password.empty();
    }

    termios original{};
    if (tcgetattr(STDIN_FILENO, &original) != 0)
    {
        return false;
    }

    termios hidden = original;
    hidden.c_lflag &= static_cast<tcflag_t>(~ECHO);
    if (tcsetattr(
            STDIN_FILENO,
            TCSAFLUSH,
            &hidden) != 0)
    {
        return false;
    }

    std::string confirmation;
    std::cerr << "New password: " << std::flush;
    const bool firstRead =
        static_cast<bool>(std::getline(std::cin, password));
    std::cerr << "\nConfirm new password: " << std::flush;
    const bool secondRead =
        static_cast<bool>(std::getline(std::cin, confirmation));
    const int restoreResult =
        tcsetattr(
            STDIN_FILENO,
            TCSAFLUSH,
            &original);
    std::cerr << std::endl;

    const bool accepted =
        firstRead &&
        secondRead &&
        restoreResult == 0 &&
        !password.empty() &&
        password == confirmation;
    secureWipe(confirmation);
    if (!accepted)
    {
        secureWipe(password);
    }
    return accepted;
}

const char* messageForStatus(HumanAccountRecoveryStatus status)
{
    switch (status)
    {
        case HumanAccountRecoveryStatus::success:
            return "";
        case HumanAccountRecoveryStatus::invalidRequest:
            return "invalid recovery request";
        case HumanAccountRecoveryStatus::accountNotFound:
            return "human account not found";
        case HumanAccountRecoveryStatus::accountInactive:
            return "human account is inactive or revoked";
        case HumanAccountRecoveryStatus::actorInvalid:
            return "human account actor binding is invalid";
        case HumanAccountRecoveryStatus::credentialNotFound:
            return "selected login credential was not found";
        case HumanAccountRecoveryStatus::credentialInvalid:
            return "selected login is not an active human-password credential for this account";
        case HumanAccountRecoveryStatus::entropyUnavailable:
            return "secure entropy is unavailable";
        case HumanAccountRecoveryStatus::hashingUnavailable:
            return "password hashing failed";
        case HumanAccountRecoveryStatus::storageError:
            return "recovery persistence failed";
    }
    return "recovery failed";
}

int exitForStatus(HumanAccountRecoveryStatus status)
{
    switch (status)
    {
        case HumanAccountRecoveryStatus::success:
            return 0;
        case HumanAccountRecoveryStatus::invalidRequest:
            return 64;
        case HumanAccountRecoveryStatus::accountNotFound:
        case HumanAccountRecoveryStatus::credentialNotFound:
            return 66;
        case HumanAccountRecoveryStatus::accountInactive:
        case HumanAccountRecoveryStatus::actorInvalid:
        case HumanAccountRecoveryStatus::credentialInvalid:
            return 77;
        case HumanAccountRecoveryStatus::entropyUnavailable:
        case HumanAccountRecoveryStatus::hashingUnavailable:
        case HumanAccountRecoveryStatus::storageError:
            return 74;
    }
    return 74;
}

std::string localRequestId()
{
    const auto ticks =
        std::chrono::system_clock::now().time_since_epoch().count();
    return "local-recovery-" +
        std::to_string(static_cast<long long>(getpid())) +
        "-" +
        std::to_string(static_cast<long long>(ticks));
}
}

int main(int argc, char** argv)
{
    if (geteuid() != 0)
    {
        std::cerr
            << "vdr-suite-human-account-recover must be run by root"
            << std::endl;
        return 77;
    }

    std::string databasePath =
        "/var/lib/vdr-suite/vdr-suite.db";
    std::string accountId;
    std::string loginName;

    for (int index = 1; index < argc; ++index)
    {
        const std::string argument = argv[index];
        if (argument == "--database" && index + 1 < argc)
        {
            databasePath = argv[++index];
        }
        else if (argument == "--account-id" &&
                 index + 1 < argc)
        {
            accountId = argv[++index];
        }
        else if (argument == "--login" &&
                 index + 1 < argc)
        {
            loginName = argv[++index];
        }
        else
        {
            printUsage();
            return 64;
        }
    }

    if (databasePath.empty() ||
        accountId.empty() ||
        loginName.empty())
    {
        printUsage();
        return 64;
    }

    std::string password;
    if (!readPassword(password))
    {
        std::cerr
            << "failed to read matching non-empty new password"
            << std::endl;
        return 64;
    }

    Database database;
    if (!database.open(databasePath))
    {
        secureWipe(password);
        std::cerr
            << "failed to open VDR-Suite security database"
            << std::endl;
        return 74;
    }

    SecurityIdentityRepository identities(database);
    HumanAccountRepository accounts(database);
    CredentialVerifierRepository verifiers(database);
    BrowserSessionCredentialRepository browserCredentials(database);
    AccountabilityEventRepository accountability(database);

    if (!identities.ensureSchema() ||
        !accounts.ensureSchema() ||
        !verifiers.ensureSchema() ||
        !browserCredentials.ensureSchema() ||
        !accountability.ensureSchema())
    {
        secureWipe(password);
        std::cerr
            << "failed to initialize Human Account recovery repositories"
            << std::endl;
        return 74;
    }

    HumanAccountRecoveryService recovery(
        database,
        accounts,
        verifiers,
        identities,
        browserCredentials,
        accountability);

    HumanAccountRecoveryRequest request;
    request.accountId = accountId;
    request.loginName = loginName;
    request.newPassword = std::move(password);
    request.requestId = localRequestId();

    const HumanAccountRecoveryResult result =
        recovery.recover(std::move(request));

    if (result.status != HumanAccountRecoveryStatus::success)
    {
        std::cerr
            << "Human Account recovery failed: "
            << messageForStatus(result.status)
            << std::endl;
        return exitForStatus(result.status);
    }

    std::cout
        << "account_id=" << result.accountId << '\n'
        << "credential_id=" << result.credentialId << '\n'
        << "revoked_browser_sessions="
        << result.revokedBrowserSessions
        << std::endl;
    return 0;
}
