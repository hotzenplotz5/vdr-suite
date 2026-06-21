#include "SearchTimer.h"

#include <cassert>
#include <iostream>

int main()
{
    SearchTimerId invalidId = SearchTimerId::fromBackendNativeId("", "");
    assert(!invalidId.isValid());

    SearchTimerId id = SearchTimerId::fromBackendNativeId("livingroom", "42");
    assert(id.isValid());
    assert(id.backendId() == "livingroom");
    assert(id.nativeId() == "42");

    SearchTimerId sameId = SearchTimerId::fromBackendNativeId("livingroom", "42");
    SearchTimerId differentBackend = SearchTimerId::fromBackendNativeId("bedroom", "42");
    SearchTimerId differentNativeId = SearchTimerId::fromBackendNativeId("livingroom", "43");

    assert(id == sameId);
    assert(id != differentBackend);
    assert(id != differentNativeId);

    SearchTimer activeTimer = SearchTimer::create(
        id,
        "Terra X",
        "Terra X",
        SearchTimerState::Active);

    assert(activeTimer.id() == id);
    assert(activeTimer.backendId() == "livingroom");
    assert(activeTimer.backendNativeId() == "42");
    assert(activeTimer.name() == "Terra X");
    assert(activeTimer.query() == "Terra X");
    assert(activeTimer.state() == SearchTimerState::Active);
    assert(activeTimer.isActive());
    assert(!activeTimer.isInactive());

    assert(activeTimer.recordingOptions().directory().empty());
    assert(activeTimer.recordingOptions().priority() == 0);
    assert(activeTimer.recordingOptions().lifetime() == 0);
    assert(activeTimer.scheduleOptions().marginStartMinutes() == 0);
    assert(activeTimer.scheduleOptions().marginStopMinutes() == 0);
    assert(!activeTimer.scheduleOptions().useVps());

    activeTimer.recordingOptions().setDirectory("Doku");
    activeTimer.recordingOptions().setPriority(50);
    activeTimer.recordingOptions().setLifetime(99);
    activeTimer.scheduleOptions().setMarginStartMinutes(5);
    activeTimer.scheduleOptions().setMarginStopMinutes(10);
    activeTimer.scheduleOptions().setUseVps(true);

    assert(activeTimer.recordingOptions().directory() == "Doku");
    assert(activeTimer.recordingOptions().priority() == 50);
    assert(activeTimer.recordingOptions().lifetime() == 99);
    assert(activeTimer.scheduleOptions().marginStartMinutes() == 5);
    assert(activeTimer.scheduleOptions().marginStopMinutes() == 10);
    assert(activeTimer.scheduleOptions().useVps());

    assert(!activeTimer.filterOptions().useChannel());
    assert(!activeTimer.filterOptions().useDayOfWeek());
    assert(!activeTimer.filterOptions().useDuration());
    assert(activeTimer.filterOptions().durationMinMinutes() == 0);
    assert(activeTimer.filterOptions().durationMaxMinutes() == 0);

    activeTimer.filterOptions().setUseChannel(true);
    activeTimer.filterOptions().setUseDayOfWeek(true);
    activeTimer.filterOptions().setUseDuration(true);
    activeTimer.filterOptions().setDurationMinMinutes(30);
    activeTimer.filterOptions().setDurationMaxMinutes(120);

    assert(activeTimer.filterOptions().useChannel());
    assert(activeTimer.filterOptions().useDayOfWeek());
    assert(activeTimer.filterOptions().useDuration());
    assert(activeTimer.filterOptions().durationMinMinutes() == 30);
    assert(activeTimer.filterOptions().durationMaxMinutes() == 120);

    assert(!activeTimer.comparisonOptions().compareTitle());
    assert(!activeTimer.comparisonOptions().compareSubtitle());
    assert(!activeTimer.comparisonOptions().compareSummary());
    assert(!activeTimer.comparisonOptions().compareCategories());
    assert(!activeTimer.comparisonOptions().compareTime());

    activeTimer.comparisonOptions().setCompareTitle(true);
    activeTimer.comparisonOptions().setCompareSubtitle(true);
    activeTimer.comparisonOptions().setCompareSummary(true);
    activeTimer.comparisonOptions().setCompareCategories(true);
    activeTimer.comparisonOptions().setCompareTime(true);

    assert(activeTimer.comparisonOptions().compareTitle());
    assert(activeTimer.comparisonOptions().compareSubtitle());
    assert(activeTimer.comparisonOptions().compareSummary());
    assert(activeTimer.comparisonOptions().compareCategories());
    assert(activeTimer.comparisonOptions().compareTime());

    assert(!activeTimer.repeatOptions().avoidRepeats());
    assert(activeTimer.repeatOptions().allowedRepeats() == 0);
    assert(activeTimer.repeatOptions().repeatsWithinDays() == 0);

    activeTimer.repeatOptions().setAvoidRepeats(true);
    activeTimer.repeatOptions().setAllowedRepeats(3);
    activeTimer.repeatOptions().setRepeatsWithinDays(14);

    assert(activeTimer.repeatOptions().avoidRepeats());
    assert(activeTimer.repeatOptions().allowedRepeats() == 3);
    assert(activeTimer.repeatOptions().repeatsWithinDays() == 14);

    assert(activeTimer.channelOptions().channels().empty());
    assert(activeTimer.channelOptions().channelMin() == 0);
    assert(activeTimer.channelOptions().channelMax() == 0);

    activeTimer.channelOptions().setChannels("1,2,3");
    activeTimer.channelOptions().setChannelMin(1);
    activeTimer.channelOptions().setChannelMax(99);

    assert(activeTimer.channelOptions().channels() == "1,2,3");
    assert(activeTimer.channelOptions().channelMin() == 1);
    assert(activeTimer.channelOptions().channelMax() == 99);

    assert(!activeTimer.seriesOptions().useSeriesRecording());
    assert(activeTimer.seriesOptions().keepRecordings() == 0);
    assert(activeTimer.seriesOptions().deleteMode() == 0);
    assert(activeTimer.seriesOptions().searchTimerAction() == 0);

    activeTimer.seriesOptions().setUseSeriesRecording(true);
    activeTimer.seriesOptions().setKeepRecordings(10);
    activeTimer.seriesOptions().setDeleteMode(2);
    activeTimer.seriesOptions().setSearchTimerAction(1);

    assert(activeTimer.seriesOptions().useSeriesRecording());
    assert(activeTimer.seriesOptions().keepRecordings() == 10);
    assert(activeTimer.seriesOptions().deleteMode() == 2);
    assert(activeTimer.seriesOptions().searchTimerAction() == 1);

    assert(activeTimer.blacklistOptions().blacklistMode() == 0);
    assert(activeTimer.blacklistOptions().blacklistIds().empty());

    activeTimer.blacklistOptions().setBlacklistMode(2);
    activeTimer.blacklistOptions().setBlacklistIds("4,5");

    assert(activeTimer.blacklistOptions().blacklistMode() == 2);
    assert(activeTimer.blacklistOptions().blacklistIds() == "4,5");

    assert(activeTimer.matchOptions().mode() == 0);
    assert(!activeTimer.matchOptions().matchCase());
    assert(activeTimer.matchOptions().tolerance() == 0);
    assert(activeTimer.matchOptions().summaryMatch() == 0);

    activeTimer.matchOptions().setMode(2);
    activeTimer.matchOptions().setMatchCase(true);
    activeTimer.matchOptions().setTolerance(1);
    activeTimer.matchOptions().setSummaryMatch(3);

    assert(activeTimer.matchOptions().mode() == 2);
    assert(activeTimer.matchOptions().matchCase());
    assert(activeTimer.matchOptions().tolerance() == 1);
    assert(activeTimer.matchOptions().summaryMatch() == 3);

    assert(!activeTimer.extendedEpgOptions().useExtendedEpgInfo());
    assert(activeTimer.extendedEpgOptions().extendedEpgInfo().empty());
    assert(!activeTimer.extendedEpgOptions().ignoreMissingEpgCategories());
    assert(activeTimer.extendedEpgOptions().contentDescriptors().empty());

    activeTimer.extendedEpgOptions().setUseExtendedEpgInfo(true);
    activeTimer.extendedEpgOptions().setExtendedEpgInfo("category=movie");
    activeTimer.extendedEpgOptions().setIgnoreMissingEpgCategories(true);
    activeTimer.extendedEpgOptions().setContentDescriptors("16,32");

    assert(activeTimer.extendedEpgOptions().useExtendedEpgInfo());
    assert(activeTimer.extendedEpgOptions().extendedEpgInfo() == "category=movie");
    assert(activeTimer.extendedEpgOptions().ignoreMissingEpgCategories());
    assert(activeTimer.extendedEpgOptions().contentDescriptors() == "16,32");

    assert(!activeTimer.validityOptions().useInFavorites());
    assert(activeTimer.validityOptions().activeFrom().empty());
    assert(activeTimer.validityOptions().activeUntil().empty());

    activeTimer.validityOptions().setUseInFavorites(true);
    activeTimer.validityOptions().setActiveFrom("2026-06-01");
    activeTimer.validityOptions().setActiveUntil("2026-12-31");

    assert(activeTimer.validityOptions().useInFavorites());
    assert(activeTimer.validityOptions().activeFrom() == "2026-06-01");
    assert(activeTimer.validityOptions().activeUntil() == "2026-12-31");

    SearchTimer inactiveTimer = SearchTimer::create(
        SearchTimerId::fromBackendNativeId("livingroom", "43"),
        "Tatort",
        "Tatort",
        SearchTimerState::Inactive);

    assert(inactiveTimer.state() == SearchTimerState::Inactive);
    assert(!inactiveTimer.isActive());
    assert(inactiveTimer.isInactive());

    std::cout << "test_search_timer passed" << std::endl;
    return 0;
}