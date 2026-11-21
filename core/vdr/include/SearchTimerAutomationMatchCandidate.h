#pragma once

#include <string>
#include <vector>

class SearchTimerAutomationMatchCandidate
{
public:
    static SearchTimerAutomationMatchCandidate createReadOnly(
        const std::string& backendId,
        const std::string& searchTimerId,
        const std::string& eventId)
    {
        SearchTimerAutomationMatchCandidate candidate;
        candidate.backendId_ = backendId;
        candidate.searchTimerId_ = searchTimerId;
        candidate.eventId_ = eventId;
        return candidate;
    }

    const std::string& backendId() const
    {
        return backendId_;
    }

    const std::string& searchTimerId() const
    {
        return searchTimerId_;
    }

    const std::string& searchTimerName() const
    {
        return searchTimerName_;
    }

    void setSearchTimerName(
        const std::string& searchTimerName)
    {
        searchTimerName_ = searchTimerName;
    }

    const std::string& eventId() const
    {
        return eventId_;
    }

    const std::string& eventTitle() const
    {
        return eventTitle_;
    }

    void setEventTitle(
        const std::string& eventTitle)
    {
        eventTitle_ = eventTitle;
    }

    const std::string& channelId() const
    {
        return channelId_;
    }

    void setChannelId(
        const std::string& channelId)
    {
        channelId_ = channelId;
    }

    long eventStartTime() const
    {
        return eventStartTime_;
    }

    void setEventStartTime(
        long eventStartTime)
    {
        eventStartTime_ = eventStartTime < 0
            ? 0
            : eventStartTime;
    }

    int eventDuration() const
    {
        return eventDuration_;
    }

    void setEventDuration(
        int eventDuration)
    {
        eventDuration_ = eventDuration < 0
            ? 0
            : eventDuration;
    }

    int matchScore() const
    {
        return matchScore_;
    }

    void setMatchScore(
        int matchScore)
    {
        if (matchScore < 0)
        {
            matchScore_ = 0;
            return;
        }

        if (matchScore > 100)
        {
            matchScore_ = 100;
            return;
        }

        matchScore_ = matchScore;
    }

    void addMatchReason(
        const std::string& reason)
    {
        if (!reason.empty())
        {
            matchReasons_.push_back(reason);
        }
    }

    const std::vector<std::string>& matchReasons() const
    {
        return matchReasons_;
    }

    bool hasMatchReasons() const
    {
        return !matchReasons_.empty();
    }

    bool dryRunOnly() const
    {
        return true;
    }

    bool mutationAllowed() const
    {
        return false;
    }

    bool timerProposalCreated() const
    {
        return false;
    }

    bool requiresDuplicateCheck() const
    {
        return true;
    }

    bool isValid() const
    {
        return !backendId_.empty()
            && !searchTimerId_.empty()
            && !eventId_.empty();
    }

    std::vector<std::string> validationErrors() const
    {
        std::vector<std::string> errors;

        if (backendId_.empty())
        {
            errors.push_back("backend id is required");
        }

        if (searchTimerId_.empty())
        {
            errors.push_back("search timer id is required");
        }

        if (eventId_.empty())
        {
            errors.push_back("event id is required");
        }

        return errors;
    }

private:
    std::string backendId_;
    std::string searchTimerId_;
    std::string searchTimerName_;
    std::string eventId_;
    std::string eventTitle_;
    std::string channelId_;
    long eventStartTime_ = 0;
    int eventDuration_ = 0;
    int matchScore_ = 0;
    std::vector<std::string> matchReasons_;
};
