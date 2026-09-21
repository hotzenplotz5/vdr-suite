#pragma once

#include "BackendAgentCommand.h"
#include "BackendAgentRecordingMarksModify.h"
#include "SecurityIdentity.h"

class BackendAgentCommandRepository;
class BackendAgentRepository;

namespace vdrsuite::agent
{

struct BackendAgentRecordingMarksModifyAssignmentRequest
{
    BackendAgentRecordingMarksModifyKind kind =
        BackendAgentRecordingMarksModifyKind::add;
    std::string operationId;
    std::string operationRevision;
    std::string recordingKey;
    std::string expectedMarksRevision;
    int sourceFrame = -1;
    int targetFrame = -1;
    std::vector<int> replacementFrames;
    std::string backendId;
    std::uint64_t backendGeneration = 0;
    std::int64_t controlPlaneClaimedAt = 0;
};

struct BackendAgentRecordingMarksModifyAssignmentResult
{
    bool accepted = false;
    bool replayed = false;
    std::string reasonCode;
    BackendAgentCommandAssignment assignment;
};

class BackendAgentRecordingMarksModifyAssignmentService
{
public:
    BackendAgentRecordingMarksModifyAssignmentService(
        BackendAgentCommandRepository&,
        BackendAgentRepository&);

    BackendAgentRecordingMarksModifyAssignmentResult assign(
        const RequestSecurityContext&,
        const BackendAgentRecordingMarksModifyAssignmentRequest&,
        std::int64_t now,
        std::int64_t deadline);

private:
    BackendAgentCommandRepository& commandRepository_;
    BackendAgentRepository& agentRepository_;
};

}
