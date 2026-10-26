#include "Database.h"
#include "ISearchTimerCommandExecutor.h"
#include "ISearchTimerDataSource.h"
#include "MutationOperationRepository.h"
#include "SearchTimerCreateAdmissionService.h"
#include "SearchTimerCreateDispatchService.h"
#include "SearchTimerCreateDeviceAdmissionService.h"

#include <cassert>
#include <iostream>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace
{
using vdrsuite::operations::MutationOperationRepository;
using vdrsuite::operations::MutationOperationState;

SearchTimerCreateAdmissionRequest input()
{
    SearchTimerCreateAdmissionRequest item;
    item.actorId = "actor:livingroom";
    item.backendId = "default";
    item.backendGeneration = 9;
    item.idempotencyKey = "remote-create-1";
    item.requestedAt = 1000;
    item.payload.name = "Terra X";
    item.payload.query = "Terra X";
    // The normal public-v1 dispatch fixture stages an inactive rule.
    // Rig(true) exercises the pre-upgrade active-operation exception.
    item.payload.active = false;
    item.payload.matchMode = 1;
    item.payload.compareTitle = true;
    item.payload.compareSummary = true;
    item.payload.marginStartMinutes = 2;
    item.payload.marginStopMinutes = 10;
    return item;
}

SearchTimer createdTimer(const SearchTimerCreateRequest& request,
    const std::string& nativeId)
{
    SearchTimer timer = SearchTimer::create(
        SearchTimerId::fromBackendNativeId(request.backendId, nativeId),
        request.name, request.query,
        request.active ? SearchTimerState::Active : SearchTimerState::Inactive);
    timer.matchOptions().setMode(request.matchMode);
    timer.matchOptions().setTolerance(request.matchTolerance);
    timer.comparisonOptions().setCompareTitle(request.compareTitle);
    timer.comparisonOptions().setCompareSubtitle(request.compareSubtitle);
    timer.comparisonOptions().setCompareSummary(request.compareSummary);
    timer.scheduleOptions().setMarginStartMinutes(request.marginStartMinutes);
    timer.scheduleOptions().setMarginStopMinutes(request.marginStopMinutes);
    return timer;
}

class Executor : public ISearchTimerCommandExecutor
{
public:
    int creates = 0;
    std::string nativeId = "31";
    bool success = true;
    SearchTimerCreateResult create(
        const SearchTimerCreateRequest& request) override
    {
        ++creates;
        last = request;
        if (!success) return SearchTimerCreateResult::failed(
            "ambiguous provider response", {"provider response lost"});
        return SearchTimerCreateResult::ok(
            createdTimer(request, nativeId), "backend replied");
    }
    SearchTimerUpdateResult update(
        const SearchTimerUpdateRequest&) override
    { return SearchTimerUpdateResult::failed("unused", {"unused"}); }
    SearchTimerDeleteResult remove(
        const SearchTimerDeleteRequest&) override
    { return SearchTimerDeleteResult::failed("unused", {"unused"}); }
    SearchTimerCreateRequest last;
};

class Generation : public ISearchTimerBackendGenerationAuthority
{
public:
    std::optional<std::uint64_t> live = 9U;
    mutable int probes = 0;
    int unavailableFromProbe = 0;
    std::optional<std::uint64_t> currentGeneration(
        const std::string& backend) const override
    {
        assert(backend == "default");
        ++probes;
        if (unavailableFromProbe > 0 && probes >= unavailableFromProbe)
            return std::nullopt;
        return live;
    }
};

class Source : public ISearchTimerDataSource
{
public:
    mutable int reads = 0;
    Generation* generation = nullptr;
    bool restartDuringBaseline = false;
    bool restartDuringReadback = false;
    std::vector<SearchTimer> initial;
    std::vector<SearchTimer> finalItems;
    bool failBefore = false;
    bool failAfter = false;
    SearchTimerResult list(const SearchTimerQuery& q) const override
    {
        ++reads;
        assert(q.backendId() == "default");
        if (reads == 1 && restartDuringBaseline && generation)
            generation->live = 10U;
        if (reads == 2 && restartDuringReadback && generation)
            generation->live = 10U;
        if ((reads == 1 && failBefore) || (reads > 1 && failAfter))
            return SearchTimerResult::unavailable(0,0);
        const auto& items = reads == 1 ? initial : finalItems;
        return SearchTimerResult::from(
            items, static_cast<int>(items.size()), 0, 0);
    }
};

struct Rig
{
    Database database;
    MutationOperationRepository storage;
    SearchTimerCreateAdmissionService admission;
    Generation generation;
    SearchTimerCreateDispatchService dispatch;
    Executor executor;
    Source source;
    std::string id;
    explicit Rig(bool historicalActive = false)
        : storage(database), admission(storage), dispatch(storage, generation)
    {
        assert(database.open(":memory:"));
        assert(storage.ensureSchema());
        source.generation = &generation;
        // Simulate durable active CREATE accepted before inactive-only API v1.
        auto admissionRequest = input();
        admissionRequest.payload.active = historicalActive;
        const auto made = admission.admit(admissionRequest);
        assert(made.status == SearchTimerCreateAdmissionStatus::prepared);
        id = made.operation.operationId;
        source.finalItems.push_back(
            createdTimer(toCommand(), executor.nativeId));
    }
    SearchTimerCreateRequest toCommand()
    {
        SearchTimerCreateRequest command;
        const auto draft = input();
        command.backendId = draft.backendId;
        command.name = draft.payload.name;
        command.query = draft.payload.query;
        command.active = draft.payload.active;
        command.matchMode = draft.payload.matchMode;
        command.matchTolerance = draft.payload.matchTolerance;
        command.compareTitle = draft.payload.compareTitle;
        command.compareSubtitle = draft.payload.compareSubtitle;
        command.compareSummary = draft.payload.compareSummary;
        command.marginStartMinutes = draft.payload.marginStartMinutes;
        command.marginStopMinutes = draft.payload.marginStopMinutes;
        return command;
    }
    SearchTimerCreateDispatchRequest request()
    {
        SearchTimerCreateDispatchRequest x;
        x.operationId = id;
        x.actorId = input().actorId;
        x.backendId = input().backendId;
        x.backendGeneration = input().backendGeneration;
        x.operatorConfirmed = true;
        x.backendWriteAllowed = true;
        x.executorOptIn = true;
        x.now = 1100;
        return x;
    }
};

void assertHappyPathAndNoRetry()
{
    Rig r;
    const auto status = r.dispatch.dispatchOnce(
        r.request(),r.executor,r.source);
    assert(status.status == SearchTimerCreateDispatchStatus::verified);
    assert(status.operation.state == MutationOperationState::succeeded);
    assert(r.executor.creates == 1);
    assert(r.executor.last.marginStartMinutes == 2);
    assert(r.executor.last.marginStopMinutes == 10);
    assert(r.source.reads == 2);
    const auto retry = r.dispatch.dispatchOnce(
        r.request(),r.executor,r.source);
    assert(retry.status == SearchTimerCreateDispatchStatus::conflict);
    assert(r.executor.creates == 1);
    assert(r.source.reads == 2);
}

void assertGuards()
{
    Rig r;
    auto req=r.request();req.backendWriteAllowed=false;
    assert(r.dispatch.dispatchOnce(req,r.executor,r.source).status ==
        SearchTimerCreateDispatchStatus::denied);
    req=r.request();req.actorId="actor:attacker";
    assert(r.dispatch.dispatchOnce(req,r.executor,r.source).status ==
        SearchTimerCreateDispatchStatus::denied);
    req=r.request();req.backendGeneration=10;
    assert(r.dispatch.dispatchOnce(req,r.executor,r.source).status ==
        SearchTimerCreateDispatchStatus::denied);
    assert(r.executor.creates == 0);
    assert(r.source.reads == 0);
    r.source.failBefore=true;
    assert(r.dispatch.dispatchOnce(r.request(),r.executor,r.source).status ==
        SearchTimerCreateDispatchStatus::notReady);
    assert(r.executor.creates == 0);
    assert(r.storage.findById(r.id).operation.state ==
        MutationOperationState::accepted);
}

void assertBackendRestartCannotDispatch()
{
    Rig stale;
    stale.generation.live = 10U;
    const auto rejected = stale.dispatch.dispatchOnce(
        stale.request(), stale.executor, stale.source);
    assert(rejected.status == SearchTimerCreateDispatchStatus::unavailable);
    assert(stale.executor.creates == 0);
    assert(stale.source.reads == 0);
    assert(stale.storage.findById(stale.id).operation.state ==
        MutationOperationState::accepted);

    Rig restarted;
    restarted.source.restartDuringBaseline = true;
    const auto afterInventory = restarted.dispatch.dispatchOnce(
        restarted.request(), restarted.executor, restarted.source);
    assert(afterInventory.status == SearchTimerCreateDispatchStatus::unavailable);
    assert(restarted.executor.creates == 0);
    assert(restarted.source.reads == 1);
    assert(restarted.storage.findById(restarted.id).operation.state ==
        MutationOperationState::accepted);

    Rig duringReadback;
    duringReadback.source.restartDuringReadback = true;
    const auto uncertain = duringReadback.dispatch.dispatchOnce(
        duringReadback.request(), duringReadback.executor, duringReadback.source);
    assert(uncertain.status == SearchTimerCreateDispatchStatus::outcomeUnknown);
    assert(duringReadback.executor.creates == 1);
    assert(duringReadback.source.reads == 2);
    assert(duringReadback.storage.findById(duringReadback.id).operation.state ==
        MutationOperationState::outcomeUnknown);
    assert(duringReadback.dispatch.dispatchOnce(
        duringReadback.request(), duringReadback.executor,
        duringReadback.source).status == SearchTimerCreateDispatchStatus::conflict);
    assert(duringReadback.executor.creates == 1);

    Rig missing;
    missing.generation.live.reset();
    assert(missing.dispatch.dispatchOnce(
        missing.request(), missing.executor, missing.source).status ==
            SearchTimerCreateDispatchStatus::unavailable);
    assert(missing.executor.creates == 0);
    assert(missing.source.reads == 0);
}

void assertLeaseExpiryCannotBeMaskedByRequestTimestamp()
{
    // A lease can expire during a slow, complete provider inventory
    // even when its backend generation does not change.
    Rig expiresDuringInventory;
    expiresDuringInventory.generation.unavailableFromProbe = 2;
    const auto beforePost = expiresDuringInventory.dispatch.dispatchOnce(
        expiresDuringInventory.request(), expiresDuringInventory.executor,
        expiresDuringInventory.source);
    assert(beforePost.status == SearchTimerCreateDispatchStatus::unavailable);
    assert(expiresDuringInventory.executor.creates == 0);
    assert(expiresDuringInventory.source.reads == 1);
    assert(expiresDuringInventory.storage.findById(
        expiresDuringInventory.id).operation.state ==
            MutationOperationState::accepted);

    // Once a POST is sent, lost authority is unknown, never success.
    Rig expiresDuringReadback;
    expiresDuringReadback.generation.unavailableFromProbe = 3;
    const auto afterPost = expiresDuringReadback.dispatch.dispatchOnce(
        expiresDuringReadback.request(), expiresDuringReadback.executor,
        expiresDuringReadback.source);
    assert(afterPost.status == SearchTimerCreateDispatchStatus::outcomeUnknown);
    assert(expiresDuringReadback.executor.creates == 1);
    assert(expiresDuringReadback.source.reads == 2);
    assert(expiresDuringReadback.storage.findById(
        expiresDuringReadback.id).operation.state ==
            MutationOperationState::outcomeUnknown);
    assert(expiresDuringReadback.dispatch.dispatchOnce(
        expiresDuringReadback.request(), expiresDuringReadback.executor,
        expiresDuringReadback.source).status ==
            SearchTimerCreateDispatchStatus::conflict);
    assert(expiresDuringReadback.executor.creates == 1);
}

void assertHistoricalActivePayloadNeverDispatches()
{
    Rig old(true);
    const auto denied = old.dispatch.dispatchOnce(
        old.request(), old.executor, old.source);
    assert(denied.status == SearchTimerCreateDispatchStatus::denied);
    assert(old.executor.creates == 0);
    assert(old.source.reads == 0);
    // No automatic recovery/redispatch of the prior active mutation.
    const auto stored = old.storage.findById(old.id);
    assert(stored.operation.state ==
        MutationOperationState::failedBeforeDispatch);
    assert(stored.operation.resultReference ==
        "searchtimer-active-create-disallowed");
    const auto again = old.dispatch.dispatchOnce(
        old.request(), old.executor, old.source);
    assert(again.status == SearchTimerCreateDispatchStatus::conflict);
    assert(old.executor.creates == 0);
    assert(old.source.reads == 0);
}

void assertAmbiguousOutcomeNeverRetries()
{
    Rig r;
    r.executor.success = false;
    const auto failed = r.dispatch.dispatchOnce(
        r.request(), r.executor, r.source);
    assert(failed.status == SearchTimerCreateDispatchStatus::outcomeUnknown);
    assert(failed.operation.state == MutationOperationState::outcomeUnknown);
    assert(r.executor.creates == 1);
    const auto retry = r.dispatch.dispatchOnce(
        r.request(), r.executor, r.source);
    assert(retry.status == SearchTimerCreateDispatchStatus::conflict);
    assert(r.executor.creates == 1);
}
void assertReadbackAndPreexistingIdentity()
{
    Rig r;
    r.source.failAfter = true;
    const auto unknown = r.dispatch.dispatchOnce(
        r.request(),r.executor,r.source);
    assert(unknown.status == SearchTimerCreateDispatchStatus::outcomeUnknown);
    assert(r.executor.creates == 1);
    assert(r.storage.findById(r.id).operation.state ==
        MutationOperationState::outcomeUnknown);

    Rig old;
    old.source.initial = old.source.finalItems;
    const auto baselineConflict = old.dispatch.dispatchOnce(
        old.request(),old.executor,old.source);
    assert(baselineConflict.status ==
        SearchTimerCreateDispatchStatus::outcomeUnknown);
    assert(old.executor.creates == 1);
    assert(old.storage.findById(old.id).operation.state ==
        MutationOperationState::outcomeUnknown);

    Rig duplicateNativeId;
    auto duplicate = duplicateNativeId.source.finalItems.front();
    duplicate.comparisonOptions().setCompareSummary(false);
    duplicateNativeId.source.finalItems.push_back(duplicate);
    const auto ambiguousIdentity = duplicateNativeId.dispatch.dispatchOnce(
        duplicateNativeId.request(),duplicateNativeId.executor,
        duplicateNativeId.source);
    assert(ambiguousIdentity.status ==
        SearchTimerCreateDispatchStatus::outcomeUnknown);
    assert(duplicateNativeId.executor.creates == 1);
    assert(duplicateNativeId.dispatch.dispatchOnce(
        duplicateNativeId.request(),duplicateNativeId.executor,
        duplicateNativeId.source).status ==
            SearchTimerCreateDispatchStatus::conflict);
    assert(duplicateNativeId.executor.creates == 1);

    Rig duplicateBaseline;
    duplicateBaseline.source.initial.push_back(
        duplicateBaseline.source.finalItems.front());
    duplicateBaseline.source.initial.push_back(
        duplicateBaseline.source.finalItems.front());
    const auto noBaseline = duplicateBaseline.dispatch.dispatchOnce(
        duplicateBaseline.request(),duplicateBaseline.executor,
        duplicateBaseline.source);
    assert(noBaseline.status == SearchTimerCreateDispatchStatus::notReady);
    assert(duplicateBaseline.executor.creates == 0);

    // An inactive rule must read back as explicitly Inactive, not
    // Unknown (isActive() alone would wrongly classify both as false).
    Rig unknownState;
    unknownState.source.finalItems.front() = SearchTimer::create(
        SearchTimerId::fromBackendNativeId(
            "default", unknownState.executor.nativeId),
        "Terra X", "Terra X", SearchTimerState::Unknown);
    const auto unknownStateResult = unknownState.dispatch.dispatchOnce(
        unknownState.request(),unknownState.executor,unknownState.source);
    assert(unknownStateResult.status ==
        SearchTimerCreateDispatchStatus::outcomeUnknown);

    Rig mismatch;
    mismatch.source.finalItems.front().comparisonOptions()
        .setCompareSummary(false);
    const auto mismatched = mismatch.dispatch.dispatchOnce(
        mismatch.request(),mismatch.executor,mismatch.source);
    assert(mismatched.status ==
        SearchTimerCreateDispatchStatus::outcomeUnknown);
    assert(mismatch.executor.creates == 1);

    Rig wrongMargin;
    wrongMargin.source.finalItems.front().scheduleOptions()
        .setMarginStopMinutes(0);
    const auto wrong = wrongMargin.dispatch.dispatchOnce(
        wrongMargin.request(), wrongMargin.executor, wrongMargin.source);
    assert(wrong.status == SearchTimerCreateDispatchStatus::outcomeUnknown);
    assert(wrongMargin.executor.creates == 1);
}
}

int main()
{
    assertHappyPathAndNoRetry();
    assertGuards();
    assertBackendRestartCannotDispatch();
    assertLeaseExpiryCannotBeMaskedByRequestTimestamp();
    assertHistoricalActivePayloadNeverDispatches();
    assertAmbiguousOutcomeNeverRetries();
    assertReadbackAndPreexistingIdentity();
    std::cout << "SearchTimer CREATE one-shot dispatch: PASS (mock only)\n";
    return 0;
}
