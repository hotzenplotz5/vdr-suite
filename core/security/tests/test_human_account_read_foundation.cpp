#include "Database.h"
#include "HumanAccountReadService.h"
#include "HumanAccountRepository.h"
#include "SecurityIdentityRepository.h"

#include <cassert>
#include <cstdio>
#include <string>

int main()
{
    const std::string path =
        "/tmp/vdr-suite-human-account-read-foundation-test.db";
    std::remove(path.c_str());

    Database database;
    assert(database.open(path));

    SecurityIdentityRepository identityRepository(database);
    assert(identityRepository.ensureSchema());

    HumanAccountRepository accountRepository(database);
    assert(accountRepository.ensureSchema());

    HumanAccountReadService readService(accountRepository);

    const auto initiallyEmpty = readService.list();
    assert(
        initiallyEmpty.status ==
        HumanAccountRepositoryStatus::ok);
    assert(initiallyEmpty.accounts.empty());

    assert(database.execute(
        "INSERT INTO security_actors "
        "(actor_id, actor_type, display_name) VALUES "
        "('human-actor-1', 'user', 'Human Actor');"));
    assert(database.execute(
        "INSERT INTO security_actors "
        "(actor_id, actor_type, display_name) VALUES "
        "('service-actor-1', 'service', 'Service Actor');"));

    assert(database.execute(
        "INSERT INTO security_human_accounts "
        "(account_id, actor_id, display_name) VALUES "
        "('account-1', 'human-actor-1', 'Primary Admin');"));

    assert(!database.execute(
        "INSERT INTO security_human_accounts "
        "(account_id, actor_id, display_name) VALUES "
        "('account-service', 'service-actor-1', 'Invalid Service');"));

    assert(!database.execute(
        "UPDATE security_actors SET actor_type = 'service' "
        "WHERE actor_id = 'human-actor-1';"));

    const auto invalid = readService.find("");
    assert(
        invalid.status ==
        HumanAccountRepositoryStatus::invalid);

    const auto missing = readService.find("missing");
    assert(
        missing.status ==
        HumanAccountRepositoryStatus::notFound);

    const auto found = readService.find("account-1");
    assert(found.status == HumanAccountRepositoryStatus::ok);
    assert(found.account.accountId == "account-1");
    assert(found.account.actorId == "human-actor-1");
    assert(found.account.displayName == "Primary Admin");
    assert(found.account.active);

    const auto list = readService.list();
    assert(list.status == HumanAccountRepositoryStatus::ok);
    assert(list.accounts.size() == 1U);
    assert(list.accounts.front().accountId == "account-1");

    assert(identityRepository.revokeActor("human-actor-1"));
    const auto revoked = readService.find("account-1");
    assert(revoked.status == HumanAccountRepositoryStatus::ok);
    assert(!revoked.account.active);

    std::remove(path.c_str());
    return 0;
}
