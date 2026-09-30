#include "Database.h"
#include "FirstAdminBootstrapIssuanceService.h"
#include "FirstAdminBootstrapRepository.h"
#include "HumanAccountRepository.h"
#include "SecurityIdentityRepository.h"
#include "SecurityPermissionGrantRepository.h"

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cctype>
#include <cstddef>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace
{
std::vector<unsigned char> bytes(
    unsigned char start,
    std::size_t size)
{
    std::vector<unsigned char> result(size);
    for (std::size_t index = 0; index < size; ++index)
    {
        result[index] =
            static_cast<unsigned char>(start + index);
    }
    return result;
}

FirstAdminBootstrapIssuanceService::EntropySource
sequenceEntropy(
    std::vector<std::vector<unsigned char>> chunks)
{
    return [chunks = std::move(chunks), index = std::size_t{0}](
               unsigned char* output,
               std::size_t size) mutable
    {
        if (output == nullptr ||
            index >= chunks.size() ||
            chunks[index].size() != size)
        {
            return false;
        }

        std::copy(
            chunks[index].begin(),
            chunks[index].end(),
            output);
        ++index;
        return true;
    };
}

bool safeSecret(const std::string& value)
{
    return value.size() >= 32 &&
        value.size() <= 256 &&
        std::all_of(
            value.begin(),
            value.end(),
            [](unsigned char character)
            {
                return std::isalnum(character) ||
                    character == '-' ||
                    character == '_';
            });
}
}

int main()
{
    static_assert(
        !std::is_copy_constructible<
            IssuedFirstAdminBootstrap>::value);
    static_assert(
        !std::is_copy_assignable<
            IssuedFirstAdminBootstrap>::value);
    static_assert(
        std::is_move_constructible<
            IssuedFirstAdminBootstrap>::value);

    Database database;
    assert(database.open(":memory:"));

    SecurityIdentityRepository identities(database);
    HumanAccountRepository accounts(database);
    SecurityPermissionGrantRepository grants(database);
    FirstAdminBootstrapRepository bootstraps(database);

    assert(identities.ensureSchema());
    assert(accounts.ensureSchema());
    assert(grants.ensureSchema());
    assert(bootstraps.ensureSchema());

    FirstAdminBootstrapIssuanceService issuer(
        bootstraps,
        sequenceEntropy({
            bytes(0x10, 16),
            bytes(0x20, 32),
            bytes(0x40, 16),
            bytes(0x60, 16),
            bytes(0x70, 32),
            bytes(0x90, 16),
        }),
        []
        {
            return std::chrono::system_clock::time_point(
                std::chrono::seconds(4070908800));
        });

    assert(
        issuer.issue(
            FirstAdminBootstrapIssuanceService::
                MinimumLifetimeSeconds - 1).status ==
        FirstAdminBootstrapIssuanceStatus::invalidLifetime);
    assert(
        issuer.issue(
            FirstAdminBootstrapIssuanceService::
                MaximumLifetimeSeconds + 1).status ==
        FirstAdminBootstrapIssuanceStatus::invalidLifetime);

    FirstAdminBootstrapIssuanceResult issued =
        issuer.issue(900);
    assert(
        issued.status ==
        FirstAdminBootstrapIssuanceStatus::issued);
    assert(issued.bootstrap.has_value());
    assert(
        issued.bootstrap->bootstrapId ==
        "fab_101112131415161718191a1b1c1d1e1f");
    assert(safeSecret(issued.bootstrap->setupSecret));
    assert(
        issued.bootstrap->expiresAt ==
        "2099-01-01 00:15:00");

    const auto stored =
        bootstraps.findById(
            issued.bootstrap->bootstrapId);
    assert(
        stored.status ==
        FirstAdminBootstrapStatus::ok);
    assert(
        stored.bootstrap.verifierHash.rfind(
            "$6$rounds=10000$",
            0) == 0);
    assert(
        stored.bootstrap.verifierHash !=
        issued.bootstrap->setupSecret);
    assert(
        stored.bootstrap.expiresAt ==
        issued.bootstrap->expiresAt);

    issued.bootstrap->clearSecret();
    assert(issued.bootstrap->setupSecret.empty());

    const FirstAdminBootstrapIssuanceResult conflict =
        issuer.issue(900);
    assert(
        conflict.status ==
        FirstAdminBootstrapIssuanceStatus::conflict);
    assert(!conflict.bootstrap.has_value());

    FirstAdminBootstrapIssuanceService entropyFailure(
        bootstraps,
        [](unsigned char*, std::size_t)
        {
            return false;
        });
    assert(
        entropyFailure.issue(900).status ==
        FirstAdminBootstrapIssuanceStatus::entropyUnavailable);

    assert(database.execute(
        "INSERT INTO security_actors "
        "(actor_id, actor_type, display_name) "
        "VALUES ('first-admin-actor', 'user', 'First administrator');"));
    assert(database.execute(
        "INSERT INTO security_human_accounts "
        "(account_id, actor_id, display_name) "
        "VALUES ('first-admin-account', 'first-admin-actor', "
        "'First administrator');"));
    assert(grants.ensureGrant(
        "first-admin-actor",
        "role.admin",
        "*"));

    FirstAdminBootstrapIssuanceService claimedIssuer(
        bootstraps,
        sequenceEntropy({
            bytes(0xa0, 16),
            bytes(0xb0, 32),
            bytes(0xd0, 16),
        }));
    const FirstAdminBootstrapIssuanceResult claimed =
        claimedIssuer.issue(900);
    assert(
        claimed.status ==
        FirstAdminBootstrapIssuanceStatus::claimed);
    assert(!claimed.bootstrap.has_value());

    return 0;
}
