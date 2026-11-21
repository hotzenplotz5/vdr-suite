#pragma once

#include "BackendAgentCommand.h"
#include "BackendAgentNativeTimerModify.h"
#include "SecurityIdentity.h"

class BackendAgentCommandRepository;
class BackendAgentRepository;

namespace vdrsuite::agent
{
struct BackendAgentNativeTimerModifyAssignmentRequest
{
    BackendAgentNativeTimerModifyKind kind =
        BackendAgentNativeTimerModifyKind::update;
    std::string operationId;
    std::string operationRevision;
    std::string timerAssignmentId;
    std::string expectedAssignmentRevision;
    std::string expectedIntentRevision;
    std::uint64_t assignmentEpoch=0;
    std::string nativeTimerBindingId;
    std::string expectedBindingRevision;
    std::string backendId;
    std::uint64_t backendGeneration=0;
    std::string backendNativeTimerId;
    std::string expectedCurrentFingerprint;
    std::string expectedSpecificationFingerprint;
    BackendAgentNativeTimerCreateSpecification specification;
    std::int64_t controlPlaneClaimedAt=0;
};
struct BackendAgentNativeTimerModifyAssignmentResult
{
    bool accepted=false;
    bool replayed=false;
    std::string reasonCode;
    BackendAgentCommandAssignment assignment;
};
class BackendAgentNativeTimerModifyAssignmentService
{
public:
    BackendAgentNativeTimerModifyAssignmentService(
        BackendAgentCommandRepository&,BackendAgentRepository&);
    BackendAgentNativeTimerModifyAssignmentResult assign(
        const RequestSecurityContext&,
        const BackendAgentNativeTimerModifyAssignmentRequest&,
        std::int64_t now,std::int64_t deadline);
private:
    BackendAgentCommandRepository& commandRepository_;
    BackendAgentRepository& agentRepository_;
};
}
