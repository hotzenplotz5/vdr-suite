#pragma once

#include "BackendAgentCommand.h"
#include "SecurityIdentity.h"

#include <cstdint>
#include <string>

class BackendAgentCommandRepository;
class BackendAgentRepository;

namespace vdrsuite::agent
{

struct BackendAgentNativeTimerDeleteAssignmentRequest
{
    std::string operationId;
    std::string operationRevision;
    std::string nativeTimerBindingId;
    std::string expectedBindingRevision;
    std::string timerAssignmentId;
    std::string backendId;
    std::uint64_t backendGeneration = 0;
    std::string backendNativeTimerId;
    std::int64_t controlPlaneClaimedAt = 0;
};

struct BackendAgentNativeTimerDeleteAssignmentResult
{
    bool accepted = false;
    bool replayed = false;
    std::string reasonCode;
    BackendAgentCommandAssignment assignment;
};

class BackendAgentNativeTimerDeleteAssignmentService
{
public:
    BackendAgentNativeTimerDeleteAssignmentService(
        BackendAgentCommandRepository& commandRepository,
        BackendAgentRepository& agentRepository);

    BackendAgentNativeTimerDeleteAssignmentResult assign(
        const RequestSecurityContext& context,
        const BackendAgentNativeTimerDeleteAssignmentRequest& request,
        std::int64_t now,
        std::int64_t deadline);

private:
    BackendAgentCommandRepository& commandRepository_;
    BackendAgentRepository& agentRepository_;
};

} // namespace vdrsuite::agent
