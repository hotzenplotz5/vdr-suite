#pragma once

#include <string>

class SearchTimerId {
public:
    static SearchTimerId fromBackendNativeId(
        const std::string& backendId,
        const std::string& nativeId)
    {
        return SearchTimerId(backendId, nativeId);
    }

    bool isValid() const
    {
        return !backendId_.empty() && !nativeId_.empty();
    }

    const std::string& backendId() const
    {
        return backendId_;
    }

    const std::string& nativeId() const
    {
        return nativeId_;
    }

    bool operator==(
        const SearchTimerId& other) const
    {
        return backendId_ == other.backendId_
            && nativeId_ == other.nativeId_;
    }

    bool operator!=(
        const SearchTimerId& other) const
    {
        return !(*this == other);
    }

private:
    SearchTimerId(
        const std::string& backendId,
        const std::string& nativeId)
        : backendId_(backendId), nativeId_(nativeId)
    {
    }

    std::string backendId_;
    std::string nativeId_;
};

enum class SearchTimerState {
    Unknown,
    Active,
    Inactive
};

class SearchTimerRecordingOptions {
public:
    const std::string& directory() const
    {
        return directory_;
    }

    int priority() const
    {
        return priority_;
    }

    int lifetime() const
    {
        return lifetime_;
    }

    void setDirectory(
        const std::string& directory)
    {
        directory_ = directory;
    }

    void setPriority(
        int priority)
    {
        priority_ = priority;
    }

    void setLifetime(
        int lifetime)
    {
        lifetime_ = lifetime;
    }

private:
    std::string directory_;
    int priority_ = 0;
    int lifetime_ = 0;
};

class SearchTimerScheduleOptions {
public:
    int marginStartMinutes() const
    {
        return marginStartMinutes_;
    }

    int marginStopMinutes() const
    {
        return marginStopMinutes_;
    }

    bool useVps() const
    {
        return useVps_;
    }

    void setMarginStartMinutes(
        int marginStartMinutes)
    {
        marginStartMinutes_ = marginStartMinutes;
    }

    void setMarginStopMinutes(
        int marginStopMinutes)
    {
        marginStopMinutes_ = marginStopMinutes;
    }

    void setUseVps(
        bool useVps)
    {
        useVps_ = useVps;
    }

private:
    int marginStartMinutes_ = 0;
    int marginStopMinutes_ = 0;
    bool useVps_ = false;
};

class SearchTimerFilterOptions {
public:
    bool useChannel() const
    {
        return useChannel_;
    }

    bool useDayOfWeek() const
    {
        return useDayOfWeek_;
    }

    bool useDuration() const
    {
        return useDuration_;
    }

    int durationMinMinutes() const
    {
        return durationMinMinutes_;
    }

    int durationMaxMinutes() const
    {
        return durationMaxMinutes_;
    }

    void setUseChannel(
        bool useChannel)
    {
        useChannel_ = useChannel;
    }

    void setUseDayOfWeek(
        bool useDayOfWeek)
    {
        useDayOfWeek_ = useDayOfWeek;
    }

    void setUseDuration(
        bool useDuration)
    {
        useDuration_ = useDuration;
    }

    void setDurationMinMinutes(
        int durationMinMinutes)
    {
        durationMinMinutes_ = durationMinMinutes;
    }

    void setDurationMaxMinutes(
        int durationMaxMinutes)
    {
        durationMaxMinutes_ = durationMaxMinutes;
    }

private:
    bool useChannel_ = false;
    bool useDayOfWeek_ = false;
    bool useDuration_ = false;
    int durationMinMinutes_ = 0;
    int durationMaxMinutes_ = 0;
};

class SearchTimerComparisonOptions {
public:
    bool compareTitle() const
    {
        return compareTitle_;
    }

    bool compareSubtitle() const
    {
        return compareSubtitle_;
    }

    bool compareSummary() const
    {
        return compareSummary_;
    }

    bool compareCategories() const
    {
        return compareCategories_;
    }

    bool compareTime() const
    {
        return compareTime_;
    }

    void setCompareTitle(bool value)
    {
        compareTitle_ = value;
    }

    void setCompareSubtitle(bool value)
    {
        compareSubtitle_ = value;
    }

    void setCompareSummary(bool value)
    {
        compareSummary_ = value;
    }

    void setCompareCategories(bool value)
    {
        compareCategories_ = value;
    }

    void setCompareTime(bool value)
    {
        compareTime_ = value;
    }

private:
    bool compareTitle_ = false;
    bool compareSubtitle_ = false;
    bool compareSummary_ = false;
    bool compareCategories_ = false;
    bool compareTime_ = false;
};

class SearchTimerRepeatOptions {
public:
    bool avoidRepeats() const
    {
        return avoidRepeats_;
    }

    int allowedRepeats() const
    {
        return allowedRepeats_;
    }

    int repeatsWithinDays() const
    {
        return repeatsWithinDays_;
    }

    void setAvoidRepeats(bool value)
    {
        avoidRepeats_ = value;
    }

    void setAllowedRepeats(int value)
    {
        allowedRepeats_ = value;
    }

    void setRepeatsWithinDays(int value)
    {
        repeatsWithinDays_ = value;
    }

private:
    bool avoidRepeats_ = false;
    int allowedRepeats_ = 0;
    int repeatsWithinDays_ = 0;
};

class SearchTimerChannelOptions {
public:
    const std::string& channels() const
    {
        return channels_;
    }

    int channelMin() const
    {
        return channelMin_;
    }

    int channelMax() const
    {
        return channelMax_;
    }

    void setChannels(const std::string& value)
    {
        channels_ = value;
    }

    void setChannelMin(int value)
    {
        channelMin_ = value;
    }

    void setChannelMax(int value)
    {
        channelMax_ = value;
    }

private:
    std::string channels_;
    int channelMin_ = 0;
    int channelMax_ = 0;
};

class SearchTimerSeriesOptions {
public:
    bool useSeriesRecording() const
    {
        return useSeriesRecording_;
    }

    int keepRecordings() const
    {
        return keepRecordings_;
    }

    int deleteMode() const
    {
        return deleteMode_;
    }

    int searchTimerAction() const
    {
        return searchTimerAction_;
    }

    void setUseSeriesRecording(bool value)
    {
        useSeriesRecording_ = value;
    }

    void setKeepRecordings(int value)
    {
        keepRecordings_ = value;
    }

    void setDeleteMode(int value)
    {
        deleteMode_ = value;
    }

    void setSearchTimerAction(int value)
    {
        searchTimerAction_ = value;
    }

private:
    bool useSeriesRecording_ = false;
    int keepRecordings_ = 0;
    int deleteMode_ = 0;
    int searchTimerAction_ = 0;
};

class SearchTimerBlacklistOptions {
public:
    int blacklistMode() const
    {
        return blacklistMode_;
    }

    const std::string& blacklistIds() const
    {
        return blacklistIds_;
    }

    void setBlacklistMode(int value)
    {
        blacklistMode_ = value;
    }

    void setBlacklistIds(const std::string& value)
    {
        blacklistIds_ = value;
    }

private:
    int blacklistMode_ = 0;
    std::string blacklistIds_;
};

class SearchTimerMatchOptions {
public:
    int mode() const
    {
        return mode_;
    }

    bool matchCase() const
    {
        return matchCase_;
    }

    int tolerance() const
    {
        return tolerance_;
    }

    int summaryMatch() const
    {
        return summaryMatch_;
    }

    void setMode(int value)
    {
        mode_ = value;
    }

    void setMatchCase(bool value)
    {
        matchCase_ = value;
    }

    void setTolerance(int value)
    {
        tolerance_ = value;
    }

    void setSummaryMatch(int value)
    {
        summaryMatch_ = value;
    }

private:
    int mode_ = 0;
    bool matchCase_ = false;
    int tolerance_ = 0;
    int summaryMatch_ = 0;
};

class SearchTimerExtendedEpgOptions {
public:
    bool useExtendedEpgInfo() const
    {
        return useExtendedEpgInfo_;
    }

    const std::string& extendedEpgInfo() const
    {
        return extendedEpgInfo_;
    }

    bool ignoreMissingEpgCategories() const
    {
        return ignoreMissingEpgCategories_;
    }

    const std::string& contentDescriptors() const
    {
        return contentDescriptors_;
    }

    void setUseExtendedEpgInfo(bool value)
    {
        useExtendedEpgInfo_ = value;
    }

    void setExtendedEpgInfo(const std::string& value)
    {
        extendedEpgInfo_ = value;
    }

    void setIgnoreMissingEpgCategories(bool value)
    {
        ignoreMissingEpgCategories_ = value;
    }

    void setContentDescriptors(const std::string& value)
    {
        contentDescriptors_ = value;
    }

private:
    bool useExtendedEpgInfo_ = false;
    std::string extendedEpgInfo_;
    bool ignoreMissingEpgCategories_ = false;
    std::string contentDescriptors_;
};

class SearchTimerValidityOptions {
public:
    bool useInFavorites() const
    {
        return useInFavorites_;
    }

    const std::string& activeFrom() const
    {
        return activeFrom_;
    }

    const std::string& activeUntil() const
    {
        return activeUntil_;
    }

    void setUseInFavorites(bool value)
    {
        useInFavorites_ = value;
    }

    void setActiveFrom(const std::string& value)
    {
        activeFrom_ = value;
    }

    void setActiveUntil(const std::string& value)
    {
        activeUntil_ = value;
    }

private:
    bool useInFavorites_ = false;
    std::string activeFrom_;
    std::string activeUntil_;
};

class SearchTimerActionOptions {
public:
    bool pauseOnRecordings() const
    {
        return pauseOnRecordings_;
    }

    int switchMinutesBefore() const
    {
        return switchMinutesBefore_;
    }

    bool unmuteSoundOnSwitch() const
    {
        return unmuteSoundOnSwitch_;
    }

    int deleteRecordingsAfterDays() const
    {
        return deleteRecordingsAfterDays_;
    }

    int deleteAfterCountRecordings() const
    {
        return deleteAfterCountRecordings_;
    }

    int deleteAfterDaysOfFirstRecording() const
    {
        return deleteAfterDaysOfFirstRecording_;
    }

    void setPauseOnRecordings(bool value)
    {
        pauseOnRecordings_ = value;
    }

    void setSwitchMinutesBefore(int value)
    {
        switchMinutesBefore_ = value;
    }

    void setUnmuteSoundOnSwitch(bool value)
    {
        unmuteSoundOnSwitch_ = value;
    }

    void setDeleteRecordingsAfterDays(int value)
    {
        deleteRecordingsAfterDays_ = value;
    }

    void setDeleteAfterCountRecordings(int value)
    {
        deleteAfterCountRecordings_ = value;
    }

    void setDeleteAfterDaysOfFirstRecording(int value)
    {
        deleteAfterDaysOfFirstRecording_ = value;
    }

private:
    bool pauseOnRecordings_ = false;
    int switchMinutesBefore_ = 0;
    bool unmuteSoundOnSwitch_ = false;
    int deleteRecordingsAfterDays_ = 0;
    int deleteAfterCountRecordings_ = 0;
    int deleteAfterDaysOfFirstRecording_ = 0;
};

class SearchTimer {
public:
    static SearchTimer create(
        const SearchTimerId& id,
        const std::string& name,
        const std::string& query,
        SearchTimerState state)
    {
        SearchTimer timer;
        timer.id_ = id;
        timer.name_ = name;
        timer.query_ = query;
        timer.state_ = state;
        return timer;
    }

    const SearchTimerId& id() const
    {
        return id_;
    }

    const std::string& backendId() const
    {
        return id_.backendId();
    }

    const std::string& backendNativeId() const
    {
        return id_.nativeId();
    }

    const std::string& name() const
    {
        return name_;
    }

    const std::string& query() const
    {
        return query_;
    }

    SearchTimerState state() const
    {
        return state_;
    }

    const SearchTimerRecordingOptions& recordingOptions() const
    {
        return recordingOptions_;
    }

    SearchTimerRecordingOptions& recordingOptions()
    {
        return recordingOptions_;
    }

    const SearchTimerScheduleOptions& scheduleOptions() const
    {
        return scheduleOptions_;
    }

    SearchTimerScheduleOptions& scheduleOptions()
    {
        return scheduleOptions_;
    }

    const SearchTimerFilterOptions& filterOptions() const
    {
        return filterOptions_;
    }

    SearchTimerFilterOptions& filterOptions()
    {
        return filterOptions_;
    }

    const SearchTimerComparisonOptions& comparisonOptions() const
    {
        return comparisonOptions_;
    }

    SearchTimerComparisonOptions& comparisonOptions()
    {
        return comparisonOptions_;
    }

    const SearchTimerRepeatOptions& repeatOptions() const
    {
        return repeatOptions_;
    }

    SearchTimerRepeatOptions& repeatOptions()
    {
        return repeatOptions_;
    }

    const SearchTimerChannelOptions& channelOptions() const
    {
        return channelOptions_;
    }

    SearchTimerChannelOptions& channelOptions()
    {
        return channelOptions_;
    }

    const SearchTimerSeriesOptions& seriesOptions() const
    {
        return seriesOptions_;
    }

    SearchTimerSeriesOptions& seriesOptions()
    {
        return seriesOptions_;
    }

    const SearchTimerBlacklistOptions& blacklistOptions() const
    {
        return blacklistOptions_;
    }

    SearchTimerBlacklistOptions& blacklistOptions()
    {
        return blacklistOptions_;
    }

    const SearchTimerMatchOptions& matchOptions() const
    {
        return matchOptions_;
    }

    SearchTimerMatchOptions& matchOptions()
    {
        return matchOptions_;
    }

    const SearchTimerExtendedEpgOptions& extendedEpgOptions() const
    {
        return extendedEpgOptions_;
    }

    SearchTimerExtendedEpgOptions& extendedEpgOptions()
    {
        return extendedEpgOptions_;
    }

    const SearchTimerValidityOptions& validityOptions() const
    {
        return validityOptions_;
    }

    SearchTimerValidityOptions& validityOptions()
    {
        return validityOptions_;
    }

    const SearchTimerActionOptions& actionOptions() const
    {
        return actionOptions_;
    }

    SearchTimerActionOptions& actionOptions()
    {
        return actionOptions_;
    }

    bool isActive() const
    {
        return state_ == SearchTimerState::Active;
    }

    bool isInactive() const
    {
        return state_ == SearchTimerState::Inactive;
    }

private:
    SearchTimerId id_ = SearchTimerId::fromBackendNativeId("", "");
    std::string name_;
    std::string query_;
    SearchTimerState state_ = SearchTimerState::Unknown;
    SearchTimerRecordingOptions recordingOptions_;
    SearchTimerScheduleOptions scheduleOptions_;
    SearchTimerFilterOptions filterOptions_;
    SearchTimerComparisonOptions comparisonOptions_;
    SearchTimerRepeatOptions repeatOptions_;
    SearchTimerChannelOptions channelOptions_;
    SearchTimerSeriesOptions seriesOptions_;
    SearchTimerBlacklistOptions blacklistOptions_;
    SearchTimerMatchOptions matchOptions_;
    SearchTimerExtendedEpgOptions extendedEpgOptions_;
    SearchTimerValidityOptions validityOptions_;
    SearchTimerActionOptions actionOptions_;
};
