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
};
