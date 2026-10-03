#pragma once

#include <cstddef>
#include <optional>
#include <string>

class Database;

class HumanAccountAdministrationRepository
{
public:
    explicit HumanAccountAdministrationRepository(Database& database);

    std::optional<std::size_t>
    countUsableAdministratorsExcludingActor(
        const std::string& excludedActorId) const;

    std::optional<std::size_t>
    countUsableAdministratorsExcludingCredential(
        const std::string& excludedCredentialId) const;

private:
    Database& database_;
};
