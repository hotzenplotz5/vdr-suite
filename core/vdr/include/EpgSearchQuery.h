#pragma once

#include <string>
#include <vector>

enum class EpgSearchMode {
    Phrase,
    AllWords,
    AnyWord,
    Exact,
    RegularExpression,
    Fuzzy
};

enum class EpgSearchChannelScope {
    Any,
    Interval,
    Group,
    FreeToAir
};

class EpgSearchQuery {
public:
    static EpgSearchQuery all()
    {
        return EpgSearchQuery();
    }

    static EpgSearchQuery byText(
        const std::string& text)
    {
        EpgSearchQuery query;
        query.text_ = text;
        return query;
    }

    EpgSearchQuery& withBackend(
        const std::string& backendId)
    {
        backendId_ = backendId;
        return *this;
    }

    EpgSearchQuery& withText(
        const std::string& text)
    {
        text_ = text;
        return *this;
    }

    EpgSearchQuery& withMode(
        EpgSearchMode mode)
    {
        mode_ = mode;
        hasMode_ = true;
        return *this;
    }

    EpgSearchQuery& withFuzzyTolerance(
        int tolerance)
    {
        fuzzyTolerance_ = tolerance;
        hasFuzzyTolerance_ = true;
        return *this;
    }

    EpgSearchQuery& searchInTitle(
        bool enabled)
    {
        useTitle_ = enabled;
        hasFieldSelection_ = true;
        return *this;
    }

    EpgSearchQuery& searchInSubtitle(
        bool enabled)
    {
        useSubtitle_ = enabled;
        hasFieldSelection_ = true;
        return *this;
    }

    EpgSearchQuery& searchInDescription(
        bool enabled)
    {
        useDescription_ = enabled;
        hasFieldSelection_ = true;
        return *this;
    }

    EpgSearchQuery& withMatchCase(
        bool enabled)
    {
        matchCase_ = enabled;
        hasMatchCase_ = true;
        return *this;
    }

    EpgSearchQuery& withChannelInterval(
        const std::string& channelMin,
        const std::string& channelMax)
    {
        channelScope_ = EpgSearchChannelScope::Interval;
        hasChannelScope_ = true;
        channelMin_ = channelMin;
        channelMax_ = channelMax;
        return *this;
    }

    EpgSearchQuery& withChannelGroup(
        const std::string& channelGroup)
    {
        channelScope_ = EpgSearchChannelScope::Group;
        hasChannelScope_ = true;
        channelGroup_ = channelGroup;
        return *this;
    }

    EpgSearchQuery& withFreeToAirOnly()
    {
        channelScope_ = EpgSearchChannelScope::FreeToAir;
        hasChannelScope_ = true;
        return *this;
    }

    EpgSearchQuery& withTimeWindow(
        int startTime,
        int stopTime)
    {
        startTime_ = startTime;
        stopTime_ = stopTime;
        hasTimeWindow_ = true;
        return *this;
    }

    EpgSearchQuery& withDurationWindow(
        int minMinutes,
        int maxMinutes)
    {
        durationMinMinutes_ = minMinutes;
        durationMaxMinutes_ = maxMinutes;
        hasDurationWindow_ = true;
        return *this;
    }

    EpgSearchQuery& withDayOfWeek(
        int dayOfWeek)
    {
        dayOfWeek_ = dayOfWeek;
        hasDayOfWeek_ = true;
        return *this;
    }

    EpgSearchQuery& withExtendedEpgInfo(
        const std::vector<std::string>& values)
    {
        extendedEpgInfo_ = values;
        hasExtendedEpgInfo_ = true;
        return *this;
    }

    EpgSearchQuery& withContentDescriptors(
        const std::string& contentDescriptors)
    {
        contentDescriptors_ = contentDescriptors;
        return *this;
    }

    EpgSearchQuery& withFavoritesOnly(
        bool enabled)
    {
        favoritesOnly_ = enabled;
        hasFavoritesOnly_ = true;
        return *this;
    }

    bool isEmpty() const
    {
        return !hasBackend()
            && !hasText()
            && !hasMode()
            && !hasFuzzyTolerance()
            && !hasFieldSelection()
            && !hasMatchCase()
            && !hasChannelScope()
            && !hasTimeWindow()
            && !hasDurationWindow()
            && !hasDayOfWeek()
            && !hasExtendedEpgInfo()
            && !hasContentDescriptors()
            && !hasFavoritesOnly();
    }

    bool matchesAll() const
    {
        return isEmpty();
    }

    bool hasBackend() const
    {
        return !backendId_.empty();
    }

    const std::string& backendId() const
    {
        return backendId_;
    }

    bool hasText() const
    {
        return !text_.empty();
    }

    const std::string& text() const
    {
        return text_;
    }

    bool hasMode() const
    {
        return hasMode_;
    }

    EpgSearchMode mode() const
    {
        return mode_;
    }

    bool hasFuzzyTolerance() const
    {
        return hasFuzzyTolerance_;
    }

    int fuzzyTolerance() const
    {
        return fuzzyTolerance_;
    }

    bool hasFieldSelection() const
    {
        return hasFieldSelection_;
    }

    bool useTitle() const
    {
        return useTitle_;
    }

    bool useSubtitle() const
    {
        return useSubtitle_;
    }

    bool useDescription() const
    {
        return useDescription_;
    }

    bool hasMatchCase() const
    {
        return hasMatchCase_;
    }

    bool matchCase() const
    {
        return matchCase_;
    }

    bool hasChannelScope() const
    {
        return hasChannelScope_;
    }

    EpgSearchChannelScope channelScope() const
    {
        return channelScope_;
    }

    const std::string& channelMin() const
    {
        return channelMin_;
    }

    const std::string& channelMax() const
    {
        return channelMax_;
    }

    const std::string& channelGroup() const
    {
        return channelGroup_;
    }

    bool hasTimeWindow() const
    {
        return hasTimeWindow_;
    }

    int startTime() const
    {
        return startTime_;
    }

    int stopTime() const
    {
        return stopTime_;
    }

    bool hasDurationWindow() const
    {
        return hasDurationWindow_;
    }

    int durationMinMinutes() const
    {
        return durationMinMinutes_;
    }

    int durationMaxMinutes() const
    {
        return durationMaxMinutes_;
    }

    bool hasDayOfWeek() const
    {
        return hasDayOfWeek_;
    }

    int dayOfWeek() const
    {
        return dayOfWeek_;
    }

    bool hasExtendedEpgInfo() const
    {
        return hasExtendedEpgInfo_;
    }

    const std::vector<std::string>& extendedEpgInfo() const
    {
        return extendedEpgInfo_;
    }

    bool hasContentDescriptors() const
    {
        return !contentDescriptors_.empty();
    }

    const std::string& contentDescriptors() const
    {
        return contentDescriptors_;
    }

    bool hasFavoritesOnly() const
    {
        return hasFavoritesOnly_;
    }

    bool favoritesOnly() const
    {
        return favoritesOnly_;
    }

private:
    std::string backendId_;
    std::string text_;

    EpgSearchMode mode_ = EpgSearchMode::Phrase;
    bool hasMode_ = false;

    int fuzzyTolerance_ = 0;
    bool hasFuzzyTolerance_ = false;

    bool useTitle_ = false;
    bool useSubtitle_ = false;
    bool useDescription_ = false;
    bool hasFieldSelection_ = false;

    bool matchCase_ = false;
    bool hasMatchCase_ = false;

    EpgSearchChannelScope channelScope_ = EpgSearchChannelScope::Any;
    bool hasChannelScope_ = false;
    std::string channelMin_;
    std::string channelMax_;
    std::string channelGroup_;

    int startTime_ = 0;
    int stopTime_ = 0;
    bool hasTimeWindow_ = false;

    int durationMinMinutes_ = 0;
    int durationMaxMinutes_ = 0;
    bool hasDurationWindow_ = false;

    int dayOfWeek_ = 0;
    bool hasDayOfWeek_ = false;

    std::vector<std::string> extendedEpgInfo_;
    bool hasExtendedEpgInfo_ = false;

    std::string contentDescriptors_;

    bool favoritesOnly_ = false;
    bool hasFavoritesOnly_ = false;
};
