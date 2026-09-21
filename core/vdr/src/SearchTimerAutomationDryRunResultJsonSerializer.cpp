#include "SearchTimerAutomationDryRunResultJsonSerializer.h"

#include <sstream>
#include <string>
#include <vector>

namespace
{

void appendQuoted(
    std::ostringstream& json,
    const std::string& value)
{
    json << '"';

    for (const char character : value)
    {
        switch (character)
        {
        case '"':
            json << "\\\"";
            break;
        case '\\':
            json << "\\\\";
            break;
        case '\n':
            json << "\\n";
            break;
        case '\r':
            json << "\\r";
            break;
        case '\t':
            json << "\\t";
            break;
        default:
            if (static_cast<unsigned char>(character) < 0x20)
            {
                json << ' ';
            }
            else
            {
                json << character;
            }
            break;
        }
    }

    json << '"';
}

void appendStringArray(
    std::ostringstream& json,
    const std::vector<std::string>& values)
{
    json << '[';

    for (std::size_t index = 0; index < values.size(); ++index)
    {
        if (index > 0)
        {
            json << ',';
        }

        appendQuoted(json, values[index]);
    }

    json << ']';
}

const char* boolText(
    bool value)
{
    return value ? "true" : "false";
}

void appendMatchCandidate(
    std::ostringstream& json,
    const SearchTimerAutomationMatchCandidate& candidate)
{
    json << "{";
    json << "\"backendId\":";
    appendQuoted(json, candidate.backendId());
    json << ",\"searchTimerId\":";
    appendQuoted(json, candidate.searchTimerId());
    json << ",\"searchTimerName\":";
    appendQuoted(json, candidate.searchTimerName());
    json << ",\"eventId\":";
    appendQuoted(json, candidate.eventId());
    json << ",\"eventTitle\":";
    appendQuoted(json, candidate.eventTitle());
    json << ",\"channelId\":";
    appendQuoted(json, candidate.channelId());
    json << ",\"eventStartTime\":" << candidate.eventStartTime();
    json << ",\"eventDuration\":" << candidate.eventDuration();
    json << ",\"matchScore\":" << candidate.matchScore();
    json << ",\"matchReasons\":";
    appendStringArray(json, candidate.matchReasons());
    json << ",\"dryRunOnly\":" << boolText(candidate.dryRunOnly());
    json << ",\"mutationAllowed\":" << boolText(candidate.mutationAllowed());
    json << ",\"timerProposalCreated\":"
         << boolText(candidate.timerProposalCreated());
    json << ",\"requiresDuplicateCheck\":"
         << boolText(candidate.requiresDuplicateCheck());
    json << "}";
}

void appendDuplicateDetection(
    std::ostringstream& json,
    const SearchTimerAutomationDuplicateDetection& duplicateDetection)
{
    json << "{";
    json << "\"backendId\":";
    appendQuoted(json, duplicateDetection.backendId());
    json << ",\"searchTimerId\":";
    appendQuoted(json, duplicateDetection.searchTimerId());
    json << ",\"eventId\":";
    appendQuoted(json, duplicateDetection.eventId());
    json << ",\"eventTitle\":";
    appendQuoted(json, duplicateDetection.eventTitle());
    json << ",\"channelId\":";
    appendQuoted(json, duplicateDetection.channelId());
    json << ",\"eventStartTime\":" << duplicateDetection.eventStartTime();
    json << ",\"eventDuration\":" << duplicateDetection.eventDuration();
    json << ",\"riskLevel\":";
    appendQuoted(
        json,
        SearchTimerAutomationDuplicateDetection::riskLevelName(
            duplicateDetection.riskLevel()));
    json << ",\"hasRisk\":" << boolText(duplicateDetection.hasRisk());
    json << ",\"requiresOperatorReview\":"
         << boolText(duplicateDetection.requiresOperatorReview());
    json << ",\"blocksAutomaticProposal\":"
         << boolText(duplicateDetection.blocksAutomaticProposal());
    json << ",\"existingTimerId\":";
    appendQuoted(json, duplicateDetection.existingTimerId());
    json << ",\"existingRecordingPath\":";
    appendQuoted(json, duplicateDetection.existingRecordingPath());
    json << ",\"titleSimilarity\":" << duplicateDetection.titleSimilarity();
    json << ",\"timeOverlapSeconds\":"
         << duplicateDetection.timeOverlapSeconds();
    json << ",\"duplicateReasons\":";
    appendStringArray(json, duplicateDetection.duplicateReasons());
    json << ",\"dryRunOnly\":" << boolText(duplicateDetection.dryRunOnly());
    json << ",\"mutationAllowed\":"
         << boolText(duplicateDetection.mutationAllowed());
    json << ",\"automaticDecisionAllowed\":"
         << boolText(duplicateDetection.automaticDecisionAllowed());
    json << ",\"timerProposalCreated\":"
         << boolText(duplicateDetection.timerProposalCreated());
    json << "}";
}

void appendCandidateTimerProposal(
    std::ostringstream& json,
    const SearchTimerAutomationCandidateTimerProposal& proposal)
{
    json << "{";
    json << "\"backendId\":";
    appendQuoted(json, proposal.backendId());
    json << ",\"searchTimerId\":";
    appendQuoted(json, proposal.searchTimerId());
    json << ",\"searchTimerName\":";
    appendQuoted(json, proposal.searchTimerName());
    json << ",\"eventId\":";
    appendQuoted(json, proposal.eventId());
    json << ",\"eventTitle\":";
    appendQuoted(json, proposal.eventTitle());
    json << ",\"channelId\":";
    appendQuoted(json, proposal.channelId());
    json << ",\"eventStartTime\":" << proposal.eventStartTime();
    json << ",\"eventDuration\":" << proposal.eventDuration();
    json << ",\"proposedStartTime\":" << proposal.proposedStartTime();
    json << ",\"proposedEndTime\":" << proposal.proposedEndTime();
    json << ",\"startMarginMinutes\":" << proposal.startMarginMinutes();
    json << ",\"stopMarginMinutes\":" << proposal.stopMarginMinutes();
    json << ",\"recordingDirectory\":";
    appendQuoted(json, proposal.recordingDirectory());
    json << ",\"priority\":" << proposal.priority();
    json << ",\"lifetime\":" << proposal.lifetime();
    json << ",\"duplicateRiskName\":";
    appendQuoted(json, proposal.duplicateRiskName());
    json << ",\"requiresOperatorReview\":"
         << boolText(proposal.requiresOperatorReview());
    json << ",\"existingTimerId\":";
    appendQuoted(json, proposal.existingTimerId());
    json << ",\"existingRecordingPath\":";
    appendQuoted(json, proposal.existingRecordingPath());
    json << ",\"proposalReasons\":";
    appendStringArray(json, proposal.proposalReasons());
    json << ",\"blocked\":" << boolText(proposal.blocked());
    json << ",\"blockReasons\":";
    appendStringArray(json, proposal.blockReasons());
    json << ",\"proposalAllowed\":" << boolText(proposal.proposalAllowed());
    json << ",\"dryRunOnly\":" << boolText(proposal.dryRunOnly());
    json << ",\"mutationAllowed\":" << boolText(proposal.mutationAllowed());
    json << ",\"timerCreationAllowed\":"
         << boolText(proposal.timerCreationAllowed());
    json << ",\"backendWriteAllowed\":"
         << boolText(proposal.backendWriteAllowed());
    json << ",\"automaticExecutionAllowed\":"
         << boolText(proposal.automaticExecutionAllowed());
    json << ",\"requiresExplicitExecutionHandoff\":"
         << boolText(proposal.requiresExplicitExecutionHandoff());
    json << "}";
}

template <typename T, typename AppendFunction>
void appendArray(
    std::ostringstream& json,
    const std::vector<T>& values,
    AppendFunction appendFunction)
{
    json << '[';

    for (std::size_t index = 0; index < values.size(); ++index)
    {
        if (index > 0)
        {
            json << ',';
        }

        appendFunction(json, values[index]);
    }

    json << ']';
}

} // namespace

std::string SearchTimerAutomationDryRunResultJsonSerializer::serialize(
    const SearchTimerAutomationDryRunResult& result) const
{
    std::ostringstream json;

    json << "{";
    json << "\"success\":" << boolText(result.successful());
    json << ",\"dryRunOnly\":" << boolText(result.dryRunOnly());
    json << ",\"mutationAllowed\":" << boolText(result.mutationAllowed());
    json << ",\"timerCreationAllowed\":"
         << boolText(result.timerCreationAllowed());
    json << ",\"backendWriteAllowed\":"
         << boolText(result.backendWriteAllowed());
    json << ",\"automaticExecutionAllowed\":"
         << boolText(result.automaticExecutionAllowed());
    json << ",\"requiresExplicitExecutionHandoff\":"
         << boolText(result.requiresExplicitExecutionHandoff());
    json << ",\"backendId\":";
    appendQuoted(json, result.backendId());
    json << ",\"candidateLimit\":" << result.candidateLimit();
    json << ",\"includeInactiveSearchTimers\":"
         << boolText(result.includeInactiveSearchTimers());
    json << ",\"includeExistingTimers\":"
         << boolText(result.includeExistingTimers());
    json << ",\"includeRecordings\":"
         << boolText(result.includeRecordings());
    json << ",\"evaluatedSearchTimerCount\":"
         << result.evaluatedSearchTimerCount();
    json << ",\"matchedCandidateCount\":" << result.matchedCandidateCount();
    json << ",\"duplicateRiskCount\":" << result.duplicateRiskCount();
    json << ",\"proposalCount\":" << result.proposalCount();
    json << ",\"allowedProposalCount\":" << result.allowedProposalCount();
    json << ",\"blockedProposalCount\":" << result.blockedProposalCount();
    json << ",\"hasContent\":" << boolText(result.hasContent());
    json << ",\"valid\":" << boolText(result.isValid());
    json << ",\"validationErrors\":";
    appendStringArray(json, result.validationErrors());
    json << ",\"warnings\":";
    appendStringArray(json, result.warnings());
    json << ",\"errors\":";
    appendStringArray(json, result.errors());
    json << ",\"auditTrail\":";
    appendStringArray(json, result.auditTrail());
    json << ",\"matchCandidates\":";
    appendArray(
        json,
        result.matchCandidates(),
        appendMatchCandidate);
    json << ",\"duplicateDetections\":";
    appendArray(
        json,
        result.duplicateDetections(),
        appendDuplicateDetection);
    json << ",\"candidateTimerProposals\":";
    appendArray(
        json,
        result.candidateTimerProposals(),
        appendCandidateTimerProposal);
    json << "}";

    return json.str();
}
