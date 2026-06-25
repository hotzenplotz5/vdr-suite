#pragma once

#include "SearchTimerAutomationDuplicateDetection.h"
#include "SearchTimerAutomationMatchCandidate.h"

#include <string>
#include <vector>

class SearchTimerAutomationCandidateTimerProposal
{
public:
    static SearchTimerAutomationCandidateTimerProposal fromCandidateAndDuplicateDetection(
        const SearchTimerAutomationMatchCandidate& candidate,
        const SearchTimerAutomationDuplicateDetection& duplicateDetection)
    {
        SearchTimerAutomationCandidateTimerProposal proposal;

        proposal.backendId_ = candidate.backendId();
        proposal.searchTimerId_ = candidate.searchTimerId();
        proposal.searchTimerName_ = candidate.searchTimerName();
        proposal.eventId_ = candidate.eventId();
        proposal.eventTitle_ = candidate.eventTitle();
        proposal.channelId_ = candidate.channelId();
        proposal.eventStartTime_ = candidate.eventStartTime();
        proposal.eventDuration_ = candidate.eventDuration();
        proposal.proposedStartTime_ = candidate.eventStartTime();
        proposal.proposedEndTime_ =
            candidate.eventStartTime() + candidate.eventDuration();
        proposal.duplicateRiskName_ =
            SearchTimerAutomationDuplicateDetection::riskLevelName(
                duplicateDetection.riskLevel());
        proposal.requiresOperatorReview_ =
            duplicateDetection.requiresOperatorReview();

        if (duplicateDetection.blocksAutomaticProposal())
        {
            proposal.markBlocked("duplicate risk blocks automatic proposal");
        }

        if (duplicateDetection.hasExistingTimer())
        {
            proposal.existingTimerId_ = duplicateDetection.existingTimerId();
        }

        if (duplicateDetection.hasExistingRecording())
        {
            proposal.existingRecordingPath_ =
                duplicateDetection.existingRecordingPath();
        }

        return proposal;
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

    long proposedStartTime() const
    {
        return proposedStartTime_;
    }

    long proposedEndTime() const
    {
        return proposedEndTime_;
    }

    int startMarginMinutes() const
    {
        return startMarginMinutes_;
    }

    int stopMarginMinutes() const
    {
        return stopMarginMinutes_;
    }

    void applyMargins(
        int startMarginMinutes,
        int stopMarginMinutes)
    {
        startMarginMinutes_ = startMarginMinutes < 0
            ? 0
            : startMarginMinutes;
        stopMarginMinutes_ = stopMarginMinutes < 0
            ? 0
            : stopMarginMinutes;

        const long startOffsetSeconds =
            static_cast<long>(startMarginMinutes_) * 60L;
        const long stopOffsetSeconds =
            static_cast<long>(stopMarginMinutes_) * 60L;

        proposedStartTime_ = eventStartTime_ > startOffsetSeconds
            ? eventStartTime_ - startOffsetSeconds
            : 0;
        proposedEndTime_ = eventStartTime_
            + static_cast<long>(eventDuration_)
            + stopOffsetSeconds;
    }

    const std::string& recordingDirectory() const
    {
        return recordingDirectory_;
    }

    void setRecordingDirectory(
        const std::string& recordingDirectory)
    {
        recordingDirectory_ = recordingDirectory;
    }

    int priority() const
    {
        return priority_;
    }

    void setPriority(
        int priority)
    {
        if (priority < 0)
        {
            priority_ = 0;
            return;
        }

        if (priority > 99)
        {
            priority_ = 99;
            return;
        }

        priority_ = priority;
    }

    int lifetime() const
    {
        return lifetime_;
    }

    void setLifetime(
        int lifetime)
    {
        if (lifetime < 0)
        {
            lifetime_ = 0;
            return;
        }

        if (lifetime > 99)
        {
            lifetime_ = 99;
            return;
        }

        lifetime_ = lifetime;
    }

    const std::string& duplicateRiskName() const
    {
        return duplicateRiskName_;
    }

    bool requiresOperatorReview() const
    {
        return requiresOperatorReview_;
    }

    void setRequiresOperatorReview(
        bool requiresOperatorReview)
    {
        requiresOperatorReview_ = requiresOperatorReview;
    }

    const std::string& existingTimerId() const
    {
        return existingTimerId_;
    }

    bool hasExistingTimer() const
    {
        return !existingTimerId_.empty();
    }

    const std::string& existingRecordingPath() const
    {
        return existingRecordingPath_;
    }

    bool hasExistingRecording() const
    {
        return !existingRecordingPath_.empty();
    }

    void addProposalReason(
        const std::string& reason)
    {
        if (!reason.empty())
        {
            proposalReasons_.push_back(reason);
        }
    }

    const std::vector<std::string>& proposalReasons() const
    {
        return proposalReasons_;
    }

    bool hasProposalReasons() const
    {
        return !proposalReasons_.empty();
    }

    void markBlocked(
        const std::string& reason)
    {
        blocked_ = true;

        if (!reason.empty())
        {
            blockReasons_.push_back(reason);
        }
    }

    bool blocked() const
    {
        return blocked_;
    }

    const std::vector<std::string>& blockReasons() const
    {
        return blockReasons_;
    }

    bool hasBlockReasons() const
    {
        return !blockReasons_.empty();
    }

    bool proposalAllowed() const
    {
        return isValid()
            && !blocked_;
    }

    bool dryRunOnly() const
    {
        return true;
    }

    bool mutationAllowed() const
    {
        return false;
    }

    bool timerCreationAllowed() const
    {
        return false;
    }

    bool backendWriteAllowed() const
    {
        return false;
    }

    bool automaticExecutionAllowed() const
    {
        return false;
    }

    bool requiresExplicitExecutionHandoff() const
    {
        return true;
    }

    bool isValid() const
    {
        return !backendId_.empty()
            && !searchTimerId_.empty()
            && !eventId_.empty()
            && !channelId_.empty()
            && proposedEndTime_ > proposedStartTime_;
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

        if (channelId_.empty())
        {
            errors.push_back("channel id is required");
        }

        if (proposedEndTime_ <= proposedStartTime_)
        {
            errors.push_back("proposal end time must be after start time");
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
    long proposedStartTime_ = 0;
    long proposedEndTime_ = 0;
    int startMarginMinutes_ = 0;
    int stopMarginMinutes_ = 0;
    std::string recordingDirectory_;
    int priority_ = 50;
    int lifetime_ = 99;
    std::string duplicateRiskName_ = "none";
    bool requiresOperatorReview_ = false;
    std::string existingTimerId_;
    std::string existingRecordingPath_;
    std::vector<std::string> proposalReasons_;
    bool blocked_ = false;
    std::vector<std::string> blockReasons_;
};
