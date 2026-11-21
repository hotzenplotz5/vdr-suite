#pragma once

#include "BackendAgentCommand.h"
#include "BackendAgentRecordingCut.h"
#include "SecurityIdentity.h"

class BackendAgentCommandRepository;
class BackendAgentRepository;

namespace vdrsuite::agent
{

struct BackendAgentRecordingCutAssignmentRequest
{
    std::string operationId;
    std::string operationRevision;
    std::string recordingKey;
    std::string expectedMarksRevision;
    std::string backendId;
    std::uint64_t backendGeneration = 0;
    std::int64_t controlPlaneClaimedAt = 0;
};

struct BackendAgentRecordingCutAssignmentResult
{
    bool accepted = false;
    bool replayed = false;
    std::string reasonCode;
    BackendAgentCommandAssignment assignment;
};

class BackendAgentRecordingCutAssignmentService
{
public:
    BackendAgentRecordingCutAssignmentService(
        BackendAgentCommandRepository&,
        BackendAgentRepository&);

    BackendAgentRecordingCutAssignmentResult assign(
        const RequestSecurityContext&,
        const BackendAgentRecordingCutAssignmentRequest&,
        std::int64_t now,
        std::int64_t deadline);

private:
    BackendAgentCommandRepository& commandRepository_;
    BackendAgentRepository& agentRepository_;
};

}
