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
    // The same Public-v1 v3 admission/dispatch supports active or inactive.
    item.payload.active = false;
    item.payload.matchMode = 1;
    item.payload.compareTitle = true;
    item.payload.compareSummary = true;
    item.payload.marginStartMinutes = 2;
    item.payload.marginStopMinutes = 10;
    item.payload.useChannel = 2;
    item.payload.channels = "ARD";
    item.payload.useTime = true;
    item.payload.startTime = 930;
    item.payload.stopTime = 1130;
    item.payload.priority = 50;
    item.payload.avoidRepeats = true;
    item.payload.allowedRepeats = 2;
    item.payload.useSeriesRecording = true;
    item.payload.blacklistIds = "4,5";
    item.payload.useExtendedEpgInfo = true;
    item.payload.extendedEpgInfo = "genre=movie";
    return item;
}

SearchTimer createdTimer(const SearchTimerCreateRequest& request,
    const std::string& nativeId)
{
    SearchTimer timer = SearchTimer::create(
        SearchTimerId::fromBackendNativeId(request.backendId, nativeId),
        request.name, request.query,
        request.active ? SearchTimerState::Active : SearchTimerState::Inactive);
    timer.recordingOptions().setDirectory(request.directory);
    timer.recordingOptions().setPriority(request.priority);
    timer.recordingOptions().setLifetime(request.lifetime);
    timer.scheduleOptions().setMarginStartMinutes(request.marginStartMinutes);
    timer.scheduleOptions().setMarginStopMinutes(request.marginStopMinutes);
    timer.scheduleOptions().setUseVps(request.useVps);
    timer.filterOptions().setChannelMode(request.useChannel);
    timer.filterOptions().setUseTime(request.useTime);
    timer.filterOptions().setStartTime(request.startTime);
    timer.filterOptions().setStopTime(request.stopTime);
    timer.filterOptions().setUseDuration(request.useDuration);
    timer.filterOptions().setDurationMinMinutes(request.durationMinMinutes);
    timer.filterOptions().setDurationMaxMinutes(request.durationMaxMinutes);
    timer.filterOptions().setUseDayOfWeek(request.useDayOfWeek);
    timer.filterOptions().setDayOfWeek(request.dayOfWeek);
    timer.channelOptions().setChannels(request.channels);
    timer.channelOptions().setChannelMinId(request.channelMin);
    timer.channelOptions().setChannelMaxId(request.channelMax);
    timer.repeatOptions().setAvoidRepeats(request.avoidRepeats);
    timer.repeatOptions().setAllowedRepeats(request.allowedRepeats);
    timer.repeatOptions().setRepeatsWithinDays(request.repeatsWithinDays);
    timer.comparisonOptions().setCompareTitle(request.compareTitle);
    timer.comparisonOptions().setCompareSubtitle(request.compareSubtitle);
    timer.comparisonOptions().setCompareSummary(request.compareSummary);
    timer.comparisonOptions().setCompareCategories(request.compareCategories);
    timer.comparisonOptions().setCompareTime(request.compareTime);
    timer.seriesOptions().setUseSeriesRecording(request.useSeriesRecording);
    timer.seriesOptions().setKeepRecordings(request.keepRecordings);
    timer.seriesOptions().setDeleteMode(request.deleteMode);
    timer.seriesOptions().setSearchTimerAction(request.searchTimerAction);
    timer.blacklistOptions().setBlacklistMode(request.blacklistMode);
    timer.blacklistOptions().setBlacklistIds(request.blacklistIds);
    timer.matchOptions().setMode(request.matchMode);
    timer.matchOptions().setMatchCase(request.matchCase);
    timer.matchOptions().setTolerance(request.matchTolerance);
    timer.matchOptions().setSummaryMatch(request.summaryMatch);
    timer.extendedEpgOptions().setUseExtendedEpgInfo(request.useExtendedEpgInfo);
    timer.extendedEpgOptions().setExtendedEpgInfo(request.extendedEpgInfo);
    timer.extendedEpgOptions().setIgnoreMissingEpgCategories(request.ignoreMissingEpgCategories);
    timer.extendedEpgOptions().setContentDescriptors(request.contentDescriptors);
    timer.validityOptions().setUseInFavorites(request.useInFavorites);
    timer.validityOptions().setActiveFrom(request.activeFrom);
    timer.validityOptions().setActiveUntil(request.activeUntil);
    timer.actionOptions().setPauseOnRecordings(request.pauseOnRecordings);
    timer.actionOptions().setSwitchMinutesBefore(request.switchMinutesBefore);
    timer.actionOptions().setUnmuteSoundOnSwitch(request.unmuteSoundOnSwitch);
    timer.actionOptions().setDeleteRecordingsAfterDays(request.deleteRecordingsAfterDays);
    timer.actionOptions().setDeleteAfterCountRecordings(request.deleteAfterCountRecordings);
    timer.actionOptions().setDeleteAfterDaysOfFirstRecording(request.deleteAfterDaysOfFirstRecording);
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
        // Exercise the same durable create path for active and inactive rules.
        auto admissionRequest = input();
        admissionRequest.payload.active = historicalActive;
        const auto made = admission.admit(admissionRequest);
        assert(made.status == SearchTimerCreateAdmissionStatus::prepared);
        id = made.operation.operationId;
        SearchTimerCreateRequest command =
            static_cast<const SearchTimerCreateRequest&>(admissionRequest.payload);
        command.backendId = admissionRequest.backendId;
        source.finalItems.push_back(createdTimer(command, executor.nativeId));
    }
    SearchTimerCreateRequest toCommand()
    {
        SearchTimerCreateRequest command =
            static_cast<const SearchTimerCreateRequest&>(input().payload);
        command.backendId = input().backendId;
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
    assert(status.operation.resultReference ==
        "searchtimer-create-verified:31");
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

void assertHttpAcceptedWithoutIdUsesSingleInventory()
{
    Rig r;
    // The provider accepted the POST but did not return an ID. The existing
    // pre-/post-inventories still establish the one new native identity.
    r.executor.nativeId.clear();
    const auto result = r.dispatch.dispatchOnce(
        r.request(), r.executor, r.source);
    assert(result.status == SearchTimerCreateDispatchStatus::verified);
    assert(result.operation.state == MutationOperationState::succeeded);
    assert(result.operation.resultReference ==
        "searchtimer-create-verified:31");
    assert(r.executor.creates == 1);
    assert(r.source.reads == 2);
    assert(r.dispatch.dispatchOnce(r.request(), r.executor, r.source).status ==
        SearchTimerCreateDispatchStatus::conflict);
    assert(r.executor.creates == 1);
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

void assertActiveCreateUsesSameOneShotPath()
{
    Rig active(true);
    const auto created = active.dispatch.dispatchOnce(
        active.request(), active.executor, active.source);
    assert(created.status == SearchTimerCreateDispatchStatus::verified);
    assert(created.operation.state == MutationOperationState::succeeded);
    assert(active.executor.creates == 1);
    assert(active.executor.last.active);
    assert(active.executor.last.priority == 50);
    assert(active.executor.last.channels == "ARD");
    assert(active.executor.last.useTime);
    assert(active.executor.last.startTime == 930);
    assert(active.executor.last.avoidRepeats);
    assert(active.executor.last.blacklistIds == "4,5");
    const auto second = active.dispatch.dispatchOnce(
        active.request(), active.executor, active.source);
    assert(second.status == SearchTimerCreateDispatchStatus::conflict);
    assert(active.executor.creates == 1);
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

    Rig wrongChannel;
    wrongChannel.source.finalItems.front().channelOptions()
        .setChannels("ZDF");
    assert(wrongChannel.dispatch.dispatchOnce(
        wrongChannel.request(), wrongChannel.executor, wrongChannel.source)
        .status == SearchTimerCreateDispatchStatus::outcomeUnknown);
    assert(wrongChannel.executor.creates == 1);

    Rig wrongPriority;
    wrongPriority.source.finalItems.front().recordingOptions()
        .setPriority(49);
    assert(wrongPriority.dispatch.dispatchOnce(
        wrongPriority.request(),wrongPriority.executor,wrongPriority.source)
        .status == SearchTimerCreateDispatchStatus::outcomeUnknown);
    assert(wrongPriority.executor.creates == 1);

    Rig wrongTime;
    wrongTime.source.finalItems.front().filterOptions().setStartTime(1000);
    assert(wrongTime.dispatch.dispatchOnce(
        wrongTime.request(),wrongTime.executor,wrongTime.source)
        .status == SearchTimerCreateDispatchStatus::outcomeUnknown);
    assert(wrongTime.executor.creates == 1);

    Rig wrongRepeats;
    wrongRepeats.source.finalItems.front().repeatOptions()
        .setAllowedRepeats(4);
    assert(wrongRepeats.dispatch.dispatchOnce(
        wrongRepeats.request(),wrongRepeats.executor,wrongRepeats.source)
        .status == SearchTimerCreateDispatchStatus::outcomeUnknown);
    assert(wrongRepeats.executor.creates == 1);

    Rig wrongBlacklist;
    wrongBlacklist.source.finalItems.front().blacklistOptions()
        .setBlacklistIds("4");
    assert(wrongBlacklist.dispatch.dispatchOnce(
        wrongBlacklist.request(),wrongBlacklist.executor,wrongBlacklist.source)
        .status == SearchTimerCreateDispatchStatus::outcomeUnknown);
    assert(wrongBlacklist.executor.creates == 1);

    Rig wrongExtendedEpg;
    wrongExtendedEpg.source.finalItems.front().extendedEpgOptions()
        .setExtendedEpgInfo("genre=documentary");
    assert(wrongExtendedEpg.dispatch.dispatchOnce(
        wrongExtendedEpg.request(),wrongExtendedEpg.executor,
        wrongExtendedEpg.source)
        .status == SearchTimerCreateDispatchStatus::outcomeUnknown);
    assert(wrongExtendedEpg.executor.creates == 1);
}
}

int main()
{
    assertHappyPathAndNoRetry();
    assertHttpAcceptedWithoutIdUsesSingleInventory();
    assertGuards();
    assertBackendRestartCannotDispatch();
    assertLeaseExpiryCannotBeMaskedByRequestTimestamp();
    assertActiveCreateUsesSameOneShotPath();
    assertAmbiguousOutcomeNeverRetries();
    assertReadbackAndPreexistingIdentity();
    std::cout << "SearchTimer CREATE one-shot dispatch: PASS (mock only)\n";
    return 0;
}
