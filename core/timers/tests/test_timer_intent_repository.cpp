#include "Database.h"
#include "TimerIntentRepository.h"

#include <cassert>
#include <iostream>
#include <string>

using namespace vdrsuite::timers;

namespace
{
TimerIntent makeIntent(const std::string& id)
{
    TimerIntent intent;
    intent.timerIntentId = id;
    intent.state = TimerIntentState::draft;
    intent.createdByActorId = "actor:create";
    intent.spec.intentType = TimerIntentType::manualWindow;
    intent.spec.ownerActorId = "actor:owner";
    intent.spec.channelRequirement.canonicalChannelId = "channel:ard";
    intent.spec.schedule.startAt = 1000;
    intent.spec.schedule.stopAt = 2000;
    intent.spec.schedule.timezone = "Europe/Berlin";
    intent.spec.recordingOptions.priority = 50;
    intent.spec.assignmentPolicy.preferredBackendIds = {
        "backend:one",
        "backend:two:with:colons"};
    intent.spec.assignmentPolicy.excludedBackendIds = {"backend:excluded"};
    intent.createdAt = 100;
    intent.updatedAt = 999;
    intent.expiresAt = 3000;
    return intent;
}
}

int main()
{
    Database database;
    assert(database.open(":memory:"));

    TimerIntentRepository repository(database);
    assert(repository.ensureSchema());

    TimerIntent input = makeIntent("intent:one");
    const auto created = repository.create(input);
    assert(created.ok());
    assert(created.intent.timerIntentId == "intent:one");
    assert(created.intent.intentRevision == "1");
    assert(created.intent.updatedAt == created.intent.createdAt);
    assert(created.intent.spec.assignmentPolicy.preferredBackendIds.size() == 2);

    const auto loaded = repository.findById("intent:one");
    assert(loaded.ok());
    assert(loaded.intent.intentRevision == "1");
    assert(loaded.intent.spec.assignmentPolicy.preferredBackendIds.at(1) ==
        "backend:two:with:colons");
    assert(timerIntentSemanticIdentity(loaded.intent.spec) ==
        timerIntentSemanticIdentity(created.intent.spec));

    const auto duplicateId = repository.create(input);
    assert(duplicateId.status == TimerIntentRepositoryStatus::alreadyExists);

    TimerIntent callerRevision = makeIntent("intent:caller-revision");
    callerRevision.intentRevision = "99";
    assert(repository.create(callerRevision).status ==
        TimerIntentRepositoryStatus::invalid);

    const auto firstEquivalent = repository.findEquivalent(created.intent.spec);
    assert(firstEquivalent.ok());
    assert(firstEquivalent.intents.size() == 1);
    assert(firstEquivalent.intents.front().timerIntentId == "intent:one");

    TimerIntent equivalent = makeIntent("intent:two");
    const auto secondCreated = repository.create(equivalent);
    assert(secondCreated.ok());
    const auto twoEquivalent = repository.findEquivalent(created.intent.spec);
    assert(twoEquivalent.ok());
    assert(twoEquivalent.intents.size() == 2);
    assert(twoEquivalent.intents.at(0).timerIntentId == "intent:one");
    assert(twoEquivalent.intents.at(1).timerIntentId == "intent:two");

    TimerIntent changed = created.intent;
    changed.spec.recordingOptions.priority = 60;
    changed.updatedAt += 1;
    const auto updated = repository.update(changed, "1");
    assert(updated.ok());
    assert(updated.intent.intentRevision == "2");
    assert(updated.intent.spec.recordingOptions.priority == 60);

    TimerIntent stale = created.intent;
    stale.spec.recordingOptions.priority = 70;
    stale.updatedAt += 2;
    const auto conflict = repository.update(stale, "1");
    assert(conflict.status == TimerIntentRepositoryStatus::conflict);
    assert(conflict.intent.intentRevision == "2");
    assert(conflict.intent.spec.recordingOptions.priority == 60);

    TimerIntent manufacturedRevision = updated.intent;
    manufacturedRevision.intentRevision = "3";
    manufacturedRevision.updatedAt += 1;
    assert(repository.update(manufacturedRevision, "2").status ==
        TimerIntentRepositoryStatus::invalid);

    TimerIntent active = updated.intent;
    active.state = TimerIntentState::active;
    active.updatedAt += 1;
    const auto activated = repository.update(active, "2");
    assert(activated.ok());
    assert(activated.intent.intentRevision == "3");
    assert(activated.intent.state == TimerIntentState::active);

    TimerIntent backwards = activated.intent;
    backwards.state = TimerIntentState::draft;
    backwards.updatedAt += 1;
    assert(repository.update(backwards, "3").status ==
        TimerIntentRepositoryStatus::invalid);

    TimerIntent satisfied = activated.intent;
    satisfied.state = TimerIntentState::satisfied;
    satisfied.updatedAt += 1;
    const auto terminal = repository.update(satisfied, "3");
    assert(terminal.ok());
    assert(terminal.intent.intentRevision == "4");

    TimerIntent mutateTerminal = terminal.intent;
    mutateTerminal.spec.recordingOptions.priority = 80;
    mutateTerminal.updatedAt += 1;
    assert(repository.update(mutateTerminal, "4").status ==
        TimerIntentRepositoryStatus::invalid);

    assert(repository.findById("missing").status ==
        TimerIntentRepositoryStatus::notFound);

    TimerIntent invalidLookup = makeIntent("unused");
    invalidLookup.spec.ownerActorId.clear();
    assert(repository.findEquivalent(invalidLookup.spec).status ==
        TimerIntentRepositoryStatus::invalid);

    std::cout << "test_timer_intent_repository passed\n";
    return 0;
}
