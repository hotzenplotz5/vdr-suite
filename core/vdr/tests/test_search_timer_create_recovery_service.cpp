#include "Database.h"
#include "MutationOperationRepository.h"
#include "SearchTimerCreateAdmissionService.h"
#include "SearchTimerCreateRecoveryService.h"

#include <cassert>
#include <cstdio>
#include <iostream>
#include <string>
#include <unistd.h>

using vdrsuite::operations::MutationOperationRepository;
using vdrsuite::operations::MutationOperationRepositoryStatus;
using vdrsuite::operations::MutationOperationState;

namespace
{
SearchTimerCreateAdmissionRequest makeAdmission()
{
    SearchTimerCreateAdmissionRequest input;
    input.actorId = "actor:device-1";
    input.backendId = "default";
    input.backendGeneration = 7U;
    input.idempotencyKey = "orphan-create-1";
    input.requestedAt = 1000;
    input.payload.name = "Terra X";
    input.payload.query = "Terra X";
    input.payload.compareTitle = true;
    return input;
}

struct Rig
{
    Database db;
    MutationOperationRepository operations;
    SearchTimerCreateAdmissionService admission;
    SearchTimerCreateRecoveryService recovery;
    std::string id;
    Rig() : operations(db), admission(operations), recovery(operations)
    {
        assert(db.open(":memory:"));
        assert(operations.ensureSchema());
        const auto accepted = admission.admit(makeAdmission());
        assert(accepted.status == SearchTimerCreateAdmissionStatus::prepared);
        id = accepted.operation.operationId;
    }

    SearchTimerCreateRecoveryRequest request() const
    {
        SearchTimerCreateRecoveryRequest req;
        req.operationId = id;
        req.actorId = "actor:device-1";
        req.backendId = "default";
        req.backendGeneration = 7U;
        req.now = 1200;
        req.staleAfterSeconds = 60;
        req.dispatcherStopped = true;
        return req;
    }

    void advance(MutationOperationState state,
        const std::string& evidence = "")
    {
        auto current = operations.findById(id);
        assert(current.ok());
        auto changed = operations.transition(
            id, current.operation.operationRevision,
            current.operation.state, state, evidence, 1100);
        assert(changed.ok());
    }
};

void assertOrphanMustBeAttestedAndExpired()
{
    Rig rig;
    rig.advance(MutationOperationState::dispatching);
    auto req = rig.request();
    req.dispatcherStopped = false;
    assert(rig.recovery.markOrphanedUnknown(req).status ==
        SearchTimerCreateRecoveryStatus::denied);
    req.dispatcherStopped = true;
    req.now = 1159;
    assert(rig.recovery.markOrphanedUnknown(req).status ==
        SearchTimerCreateRecoveryStatus::notExpired);
    req.now = 1099;
    assert(rig.recovery.markOrphanedUnknown(req).status ==
        SearchTimerCreateRecoveryStatus::notExpired);
    req.now = 1160;
    assert(rig.recovery.markOrphanedUnknown(req).status ==
        SearchTimerCreateRecoveryStatus::markedUnknown);
    const auto persisted = rig.operations.findById(rig.id);
    assert(persisted.ok());
    assert(persisted.operation.state == MutationOperationState::outcomeUnknown);
    assert(rig.recovery.markOrphanedUnknown(req).status ==
        SearchTimerCreateRecoveryStatus::alreadyUnknown);
    // A replay of CREATE admission can only return the old operation ID.
    const auto replayed = rig.admission.admit(makeAdmission());
    assert(replayed.status == SearchTimerCreateAdmissionStatus::replayed);
    assert(replayed.operation.operationId == rig.id);
    assert(replayed.operation.state == MutationOperationState::outcomeUnknown);
}

void assertWrongAuthorityAndUnsentNeverChange()
{
    Rig rig;
    auto req = rig.request();
    // An accepted operation has never claimed the provider effect and must
    // stay accepted: no manufactured dispatch or outcome.
    assert(rig.recovery.markOrphanedUnknown(req).status ==
        SearchTimerCreateRecoveryStatus::conflict);
    rig.advance(MutationOperationState::dispatching);
    req.actorId = "actor:attacker";
    assert(rig.recovery.markOrphanedUnknown(req).status ==
        SearchTimerCreateRecoveryStatus::denied);
    req = rig.request(); req.backendId = "other";
    assert(rig.recovery.markOrphanedUnknown(req).status ==
        SearchTimerCreateRecoveryStatus::denied);
    req = rig.request(); req.backendGeneration = 8U;
    assert(rig.recovery.markOrphanedUnknown(req).status ==
        SearchTimerCreateRecoveryStatus::denied);
    req = rig.request(); req.staleAfterSeconds = 0;
    assert(rig.recovery.markOrphanedUnknown(req).status ==
        SearchTimerCreateRecoveryStatus::denied);
    req = rig.request(); req.staleAfterSeconds = 86401;
    assert(rig.recovery.markOrphanedUnknown(req).status ==
        SearchTimerCreateRecoveryStatus::denied);
    assert(rig.operations.findById(rig.id).operation.state ==
        MutationOperationState::dispatching);
}

void assertKeepDurableReadbackEvidence()
{
    Rig rig;
    rig.advance(MutationOperationState::dispatching);
    rig.advance(MutationOperationState::executedUnverified,
        "opaque-readback-evidence");
    const auto recovered = rig.recovery.markOrphanedUnknown(rig.request());
    assert(recovered.status == SearchTimerCreateRecoveryStatus::markedUnknown);
    assert(recovered.operation.resultReference ==
        "opaque-readback-evidence");
    const auto original = rig.operations.findById(rig.id);
    // A worker still holding the previous revision can never finish it.
    assert(rig.operations.transition(
        rig.id,"2",MutationOperationState::dispatching,
        MutationOperationState::executedUnverified,"",1201).status ==
        MutationOperationRepositoryStatus::revisionConflict);
    assert(original.operation.state == MutationOperationState::outcomeUnknown);
    assert(rig.operations.transition(
        rig.id,original.operation.operationRevision,
        MutationOperationState::outcomeUnknown,
        MutationOperationState::accepted,"",1202).status ==
        MutationOperationRepositoryStatus::stateConflict);
}

void assertFileBackedRecoverySurvivesRestart()
{
    char filename[] = "/tmp/vdr-suite-searchtimer-recovery-XXXXXX";
    const int fd = mkstemp(filename);
    assert(fd >= 0);
    assert(close(fd) == 0);
    std::string id;
    {
        Database db;
        assert(db.open(filename));
        MutationOperationRepository storage(db);
        assert(storage.ensureSchema());
        SearchTimerCreateAdmissionService admission(storage);
        const auto first = admission.admit(makeAdmission());
        assert(first.status == SearchTimerCreateAdmissionStatus::prepared);
        id = first.operation.operationId;
        const auto claimed = storage.transition(
            id, first.operation.operationRevision,
            MutationOperationState::accepted,
            MutationOperationState::dispatching,"",1100);
        assert(claimed.ok());
    }
    {
        Database db;
        assert(db.open(filename));
        MutationOperationRepository storage(db);
        assert(storage.ensureSchema());
        SearchTimerCreateRecoveryService recovery(storage);
        SearchTimerCreateRecoveryRequest req;
        req.operationId = id;
        req.actorId = "actor:device-1";
        req.backendId = "default";
        req.backendGeneration = 7U;
        req.now = 1300;
        req.staleAfterSeconds = 60;
        req.dispatcherStopped = true;
        assert(recovery.markOrphanedUnknown(req).status ==
            SearchTimerCreateRecoveryStatus::markedUnknown);
    }
    {
        Database db;
        assert(db.open(filename));
        MutationOperationRepository storage(db);
        assert(storage.ensureSchema());
        assert(storage.findById(id).operation.state ==
            MutationOperationState::outcomeUnknown);
        SearchTimerCreateAdmissionService admission(storage);
        assert(admission.admit(makeAdmission()).status ==
            SearchTimerCreateAdmissionStatus::replayed);
    }
    assert(std::remove(filename) == 0);
}
}

int main()
{
    assertOrphanMustBeAttestedAndExpired();
    assertWrongAuthorityAndUnsentNeverChange();
    assertKeepDurableReadbackEvidence();
    assertFileBackedRecoverySurvivesRestart();
    std::cout << "SearchTimer CREATE orphan recovery: PASS (no provider calls)\n";
    return 0;
}
