#include "Database.h"
#include "MutationOperationRepository.h"
#include "SearchTimerNativeCasDispatchService.h"
#include "SearchTimerCreateDeviceAdmissionService.h"

#include <cassert>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>

namespace
{
using namespace vdrsuite::operations;

constexpr const char* kGen = "sg_aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
constexpr const char* kRev = "sr_bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb";
constexpr const char* kNewRev = "sr_cccccccccccccccccccccccccccccccc";

struct Provider final : ISearchTimerNativeCasProvider
{
    bool capability = true;
    bool loseReply = false;
    bool noWrite = false;
    int writes = 0;
    SearchTimerNativeCasStatus nativeStatus = SearchTimerNativeCasStatus::applied;

    bool supportsAtomicGenerationCasV1() const override
    { return capability; }
    SearchTimerNativeCasReply compareAndWrite(
        const SearchTimerNativeCasRequest& request) override
    {
        ++writes;
        assert(request.durableOperationClaimed);
        assert(request.backendWriteAuthorized);
        assert(request.operationId ==
            "op_11111111111111111111111111111111");
        if (loseReply)
            throw std::runtime_error("reply lost after write");
        SearchTimerNativeCasReply reply;
        reply.status = nativeStatus;
        reply.confirmedGeneration = kGen;
        reply.atomicCompareAndWrite = true;
        reply.noWriteAttested = noWrite;
        if (nativeStatus == SearchTimerNativeCasStatus::applied)
            reply.resultingRevision = kNewRev;
        return reply;
    }
};

struct Generation final : ISearchTimerBackendGenerationAuthority
{
    std::optional<std::uint64_t> live = 9U;
    std::optional<std::uint64_t> currentGeneration(
        const std::string& backend) const override
    {
        assert(backend == "default");
        return live;
    }
};

struct Fixture
{
    Database database;
    MutationOperationRepository operations;
    Generation generation;
    SearchTimerNativeCasDispatchService dispatch;
    SearchTimerNativeCasDispatchRequest req;
    Provider provider;

    Fixture() : operations(database), dispatch(operations, generation)
    {
        assert(database.open(":memory:"));
        assert(operations.ensureSchema());
        auto& native = req.native;
        native.action = SearchTimerNativeCasAction::activate;
        native.backendId = "default";
        native.publicSearchTimerId =
            "st_22222222222222222222222222222222";
        native.nativeId = "0";
        native.expectedGeneration = kGen;
        native.expectedRevision = kRev;
        // Untrusted request flags should have no bearing on admission.
        native.durableOperationClaimed = false;
        native.backendWriteAuthorized = false;
        native.operatorConfirmed = false;
        req.operationId = "op_11111111111111111111111111111111";
        req.actorId = "actor:livingroom";
        req.backendGeneration = 9;
        req.now = 1100;
        req.backendWriteAuthorized = true;
        req.operatorConfirmed = true;

        const auto evidence = serializeSearchTimerNativeCasEvidence(native);
        const auto fingerprint =
            searchTimerNativeCasEvidenceFingerprint(native);
        assert(!evidence.empty());
        assert(fingerprint.size() == 71U);
        MutationOperation op;
        op.operationId = req.operationId;
        op.idempotencyKey = "cas-1";
        op.actorId = req.actorId;
        op.backendId = native.backendId;
        op.backendGeneration = req.backendGeneration;
        op.resourceType = "SearchTimer";
        op.resourceId = native.publicSearchTimerId;
        op.expectedRevision = native.expectedRevision;
        op.expectedResourceFingerprint = native.expectedGeneration;
        op.actionFamily = "searchtimer.native-cas.activate";
        op.requestFingerprint = fingerprint;
        op.requestedAt = 1000;
        op.updatedAt = 1000;
        op.verificationPolicy =
            MutationOperationVerificationPolicy::readbackRequired;
        MutationOperationPayload stored;
        stored.operationId = op.operationId;
        stored.payloadType = "searchtimer.native-cas";
        stored.payloadVersion = 1U;
        stored.payload = evidence;
        stored.payloadFingerprint = fingerprint;
        const auto made = operations.reserveWithPayload(op, stored);
        assert(made.status == MutationOperationRepositoryStatus::ok);
    }

    MutationOperationState state()
    {
        return operations.findById(req.operationId).operation.state;
    }
};

void assertOneShotAndReadbackFence()
{
    Fixture r;
    const auto completed = r.dispatch.dispatchOnce(r.req, r.provider);
    assert(completed.status ==
        SearchTimerNativeCasDispatchStatus::dispatchedUnverified);
    assert(completed.nativeStatus == SearchTimerNativeCasStatus::applied);
    assert(completed.operation.state ==
        MutationOperationState::executedUnverified);
    assert(r.provider.writes == 1);
    assert(r.dispatch.dispatchOnce(r.req, r.provider).status ==
        SearchTimerNativeCasDispatchStatus::conflict);
    assert(r.provider.writes == 1);
}

void assertImmutableEvidenceAndActorScope()
{
    Fixture r;
    auto wrong = r.req;
    wrong.actorId = "actor:attacker";
    assert(r.dispatch.dispatchOnce(wrong, r.provider).status ==
        SearchTimerNativeCasDispatchStatus::denied);
    wrong = r.req;
    wrong.backendGeneration = 10;
    assert(r.dispatch.dispatchOnce(wrong, r.provider).status ==
        SearchTimerNativeCasDispatchStatus::denied);
    wrong = r.req;
    wrong.native.nativeId = "1";
    assert(r.dispatch.dispatchOnce(wrong, r.provider).status ==
        SearchTimerNativeCasDispatchStatus::denied);
    wrong = r.req;
    wrong.native.expectedGeneration =
        "sg_ffffffffffffffffffffffffffffffff";
    assert(r.dispatch.dispatchOnce(wrong, r.provider).status ==
        SearchTimerNativeCasDispatchStatus::denied);
    wrong = r.req;
    wrong.native.changePayload = "unauthorized update";
    assert(r.dispatch.dispatchOnce(wrong, r.provider).status ==
        SearchTimerNativeCasDispatchStatus::denied);
    wrong = r.req;
    wrong.operatorConfirmed = false;
    assert(r.dispatch.dispatchOnce(wrong, r.provider).status ==
        SearchTimerNativeCasDispatchStatus::denied);
    wrong = r.req;
    wrong.backendWriteAuthorized = false;
    assert(r.dispatch.dispatchOnce(wrong, r.provider).status ==
        SearchTimerNativeCasDispatchStatus::denied);
    assert(r.provider.writes == 0);
    assert(r.state() == MutationOperationState::accepted);
}

void assertBackendGenerationRevalidation()
{
    Fixture replaced;
    replaced.generation.live = 10U;
    assert(replaced.dispatch.dispatchOnce(
        replaced.req, replaced.provider).status ==
        SearchTimerNativeCasDispatchStatus::unavailable);
    assert(replaced.provider.writes == 0);
    assert(replaced.state() == MutationOperationState::accepted);

    Fixture offline;
    offline.generation.live.reset();
    assert(offline.dispatch.dispatchOnce(offline.req, offline.provider).status ==
        SearchTimerNativeCasDispatchStatus::unavailable);
    assert(offline.provider.writes == 0);
    assert(offline.state() == MutationOperationState::accepted);
}

void assertCapabilityAndNegativeAttestation()
{
    Fixture missing;
    missing.provider.capability = false;
    assert(missing.dispatch.dispatchOnce(missing.req, missing.provider).status ==
        SearchTimerNativeCasDispatchStatus::unsupported);
    assert(missing.provider.writes == 0);
    assert(missing.state() == MutationOperationState::accepted);

    Fixture stale;
    stale.provider.nativeStatus = SearchTimerNativeCasStatus::staleGeneration;
    stale.provider.noWrite = true;
    const auto denied = stale.dispatch.dispatchOnce(stale.req, stale.provider);
    assert(denied.status == SearchTimerNativeCasDispatchStatus::failedVerified);
    assert(denied.nativeStatus == SearchTimerNativeCasStatus::staleGeneration);
    assert(stale.state() == MutationOperationState::failedVerified);
    assert(stale.provider.writes == 1);
    assert(stale.dispatch.dispatchOnce(stale.req, stale.provider).status ==
        SearchTimerNativeCasDispatchStatus::conflict);
    assert(stale.provider.writes == 1);

    Fixture unproven;
    unproven.provider.nativeStatus = SearchTimerNativeCasStatus::staleGeneration;
    const auto unknown = unproven.dispatch.dispatchOnce(
        unproven.req, unproven.provider);
    assert(unknown.status == SearchTimerNativeCasDispatchStatus::outcomeUnknown);
    assert(unproven.state() == MutationOperationState::outcomeUnknown);
}

void assertLostReceiptDoesNotRepeat()
{
    Fixture r;
    r.provider.loseReply = true;
    assert(r.dispatch.dispatchOnce(r.req, r.provider).status ==
        SearchTimerNativeCasDispatchStatus::outcomeUnknown);
    assert(r.state() == MutationOperationState::outcomeUnknown);
    assert(r.dispatch.dispatchOnce(r.req, r.provider).status ==
        SearchTimerNativeCasDispatchStatus::conflict);
    assert(r.provider.writes == 1);
}
} // namespace

int main()
{
    assertOneShotAndReadbackFence();
    assertImmutableEvidenceAndActorScope();
    assertBackendGenerationRevalidation();
    assertCapabilityAndNegativeAttestation();
    assertLostReceiptDoesNotRepeat();
    std::cout << "SearchTimer native CAS durable dispatch: PASS (mock only)\n";
}
