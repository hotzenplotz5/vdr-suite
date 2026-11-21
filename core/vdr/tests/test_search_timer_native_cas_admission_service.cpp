#include "Database.h"
#include "MutationOperationRepository.h"
#include "SearchTimerNativeCasAdmissionService.h"
#include "SearchTimerNativeCasDispatchService.h"

#include <cassert>
#include <iostream>
#include <string>

namespace
{
using namespace vdrsuite::operations;

SearchTimerNativeCasAdmissionRequest fixture()
{
    SearchTimerNativeCasAdmissionRequest req;
    req.actorId = "actor:livingroom";
    req.backendGeneration = 9;
    req.idempotencyKey = "searchtimer-change-1";
    req.requestedAt = 1000;
    req.native.action = SearchTimerNativeCasAction::deactivate;
    req.native.backendId = "default";
    req.native.publicSearchTimerId =
        "st_22222222222222222222222222222222";
    req.native.nativeId = "0";
    req.native.expectedGeneration =
        "sg_aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
    req.native.expectedRevision =
        "sr_bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb";
    return req;
}

void assertAdmissionAndReplay()
{
    Database db;
    assert(db.open(":memory:"));
    MutationOperationRepository repo(db);
    assert(repo.ensureSchema());
    SearchTimerNativeCasAdmissionService admission(repo);
    const auto req = fixture();
    const auto made = admission.admit(req);
    assert(made.status == SearchTimerNativeCasAdmissionStatus::prepared);
    assert(made.operation.backendGeneration == 9U);
    assert(made.operation.resourceId == req.native.publicSearchTimerId);
    assert(made.operation.expectedRevision == req.native.expectedRevision);
    assert(made.operation.expectedResourceFingerprint ==
        req.native.expectedGeneration);
    assert(made.operation.operationRevision == "1");
    const auto stored = repo.findPayloadByOperationId(made.operation.operationId);
    assert(stored.ok());
    assert(stored.payload.payloadType == "searchtimer.native-cas");
    assert(stored.payload.payloadVersion == 1U);
    assert(stored.payload.payload == serializeSearchTimerNativeCasEvidence(
        req.native));
    assert(stored.payload.payloadFingerprint ==
        searchTimerNativeCasEvidenceFingerprint(req.native));
    const auto same = admission.admit(req);
    assert(same.status == SearchTimerNativeCasAdmissionStatus::replayed);
    assert(same.operation.operationId == made.operation.operationId);
    assert(repo.findById(made.operation.operationId).operation.state ==
        MutationOperationState::accepted);

    auto changed = req;
    changed.native.expectedGeneration =
        "sg_cccccccccccccccccccccccccccccccc";
    assert(admission.admit(changed).status !=
        SearchTimerNativeCasAdmissionStatus::prepared);
    changed = req;
    changed.backendGeneration = 10;
    const auto historic = admission.admit(changed);
    assert(historic.status == SearchTimerNativeCasAdmissionStatus::replayed);
    assert(historic.operation.backendGeneration == 9U);
}

void assertInvalidNeverPersists()
{
    Database db;
    assert(db.open(":memory:"));
    MutationOperationRepository repo(db);
    assert(repo.ensureSchema());
    SearchTimerNativeCasAdmissionService admission(repo);
    auto req = fixture();
    req.native.nativeId = "00";
    assert(admission.admit(req).status ==
        SearchTimerNativeCasAdmissionStatus::invalid);
    req = fixture();
    req.native.expectedRevision.clear();
    assert(admission.admit(req).status ==
        SearchTimerNativeCasAdmissionStatus::invalid);
    req = fixture();
    req.native.action = SearchTimerNativeCasAction::update;
    assert(admission.admit(req).status ==
        SearchTimerNativeCasAdmissionStatus::invalid);
    req.native.changePayload = "updated settings";
    assert(admission.admit(req).status ==
        SearchTimerNativeCasAdmissionStatus::prepared);
}
}

int main()
{
    assertAdmissionAndReplay();
    assertInvalidNeverPersists();
    std::cout << "SearchTimer native CAS durable admission: PASS (mock only)\n";
}
