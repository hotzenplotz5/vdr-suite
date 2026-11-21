#pragma once

#include "SearchTimerAutomationMatchCandidate.h"

#include <string>
#include <vector>

enum class SearchTimerAutomationDuplicateRiskLevel
{
    None,
    Low,
    Medium,
    High,
    Blocking
};

class SearchTimerAutomationDuplicateDetection
{
public:
    static SearchTimerAutomationDuplicateDetection forCandidate(
        const SearchTimerAutomationMatchCandidate& candidate)
    {
        SearchTimerAutomationDuplicateDetection detection;
        detection.backendId_ = candidate.backendId();
        detection.searchTimerId_ = candidate.searchTimerId();
        detection.eventId_ = candidate.eventId();
        detection.eventTitle_ = candidate.eventTitle();
        detection.channelId_ = candidate.channelId();
        detection.eventStartTime_ = candidate.eventStartTime();
        detection.eventDuration_ = candidate.eventDuration();
        return detection;
    }

    const std::string& backendId() const
    {
        return backendId_;
    }

    const std::string& searchTimerId() const
    {
        return searchTimerId_;
    }

    const std::string& eventId() const
    {
        return eventId_;
    }

    const std::string& eventTitle() const
    {
        return eventTitle_;
    }

    const std::string& channelId() const
    {
        return channelId_;
    }

    long eventStartTime() const
    {
        return eventStartTime_;
    }

    int eventDuration() const
    {
        return eventDuration_;
    }

    SearchTimerAutomationDuplicateRiskLevel riskLevel() const
    {
        return riskLevel_;
    }

    void setRiskLevel(
        SearchTimerAutomationDuplicateRiskLevel riskLevel)
    {
        riskLevel_ = riskLevel;
    }

    bool hasRisk() const
    {
        return riskLevel_ != SearchTimerAutomationDuplicateRiskLevel::None;
    }

    bool blocksAutomaticProposal() const
    {
        return riskLevel_ == SearchTimerAutomationDuplicateRiskLevel::High
            || riskLevel_ == SearchTimerAutomationDuplicateRiskLevel::Blocking;
    }

    bool requiresOperatorReview() const
    {
        return riskLevel_ == SearchTimerAutomationDuplicateRiskLevel::Medium
            || riskLevel_ == SearchTimerAutomationDuplicateRiskLevel::High
            || riskLevel_ == SearchTimerAutomationDuplicateRiskLevel::Blocking;
    }

    const std::string& existingTimerId() const
    {
        return existingTimerId_;
    }

    void setExistingTimerId(
        const std::string& existingTimerId)
    {
        existingTimerId_ = existingTimerId;
    }

    bool hasExistingTimer() const
    {
        return !existingTimerId_.empty();
    }

    const std::string& existingRecordingPath() const
    {
        return existingRecordingPath_;
    }

    void setExistingRecordingPath(
        const std::string& existingRecordingPath)
    {
        existingRecordingPath_ = existingRecordingPath;
    }

    bool hasExistingRecording() const
    {
        return !existingRecordingPath_.empty();
    }

    int titleSimilarity() const
    {
        return titleSimilarity_;
    }

    void setTitleSimilarity(
        int titleSimilarity)
    {
        if (titleSimilarity < 0)
        {
            titleSimilarity_ = 0;
            return;
        }

        if (titleSimilarity > 100)
        {
            titleSimilarity_ = 100;
            return;
        }

        titleSimilarity_ = titleSimilarity;
    }

    int timeOverlapSeconds() const
    {
        return timeOverlapSeconds_;
    }

    void setTimeOverlapSeconds(
        int timeOverlapSeconds)
    {
        timeOverlapSeconds_ = timeOverlapSeconds < 0
            ? 0
            : timeOverlapSeconds;
    }

    void addDuplicateReason(
        const std::string& reason)
    {
        if (!reason.empty())
        {
            duplicateReasons_.push_back(reason);
        }
    }

    const std::vector<std::string>& duplicateReasons() const
    {
        return duplicateReasons_;
    }

    bool hasDuplicateReasons() const
    {
        return !duplicateReasons_.empty();
    }

    bool dryRunOnly() const
    {
        return true;
    }

    bool mutationAllowed() const
    {
        return false;
    }

    bool automaticDecisionAllowed() const
    {
        return false;
    }

    bool timerProposalCreated() const
    {
        return false;
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

    static std::string riskLevelName(
        SearchTimerAutomationDuplicateRiskLevel riskLevel)
    {
        switch (riskLevel)
        {
        case SearchTimerAutomationDuplicateRiskLevel::None:
            return "none";
        case SearchTimerAutomationDuplicateRiskLevel::Low:
            return "low";
        case SearchTimerAutomationDuplicateRiskLevel::Medium:
            return "medium";
        case SearchTimerAutomationDuplicateRiskLevel::High:
            return "high";
        case SearchTimerAutomationDuplicateRiskLevel::Blocking:
            return "blocking";
        }

        return "unknown";
    }

private:
    std::string backendId_;
    std::string searchTimerId_;
    std::string eventId_;
    std::string eventTitle_;
    std::string channelId_;
    long eventStartTime_ = 0;
    int eventDuration_ = 0;
    SearchTimerAutomationDuplicateRiskLevel riskLevel_ =
        SearchTimerAutomationDuplicateRiskLevel::None;
    std::string existingTimerId_;
    std::string existingRecordingPath_;
    int titleSimilarity_ = 0;
    int timeOverlapSeconds_ = 0;
    std::vector<std::string> duplicateReasons_;
};
