#pragma once

#include "BackendAgentCommand.h"
#include "BackendAgentNativeTimerCreate.h"
#include "SecurityIdentity.h"

#include <cstdint>
#include <string>

class BackendAgentCommandRepository;
class BackendAgentCommandReservationRepository;
class BackendAgentRepository;

namespace vdrsuite::agent
{

struct BackendAgentNativeTimerCreateReservationRequest
{
    std::string operationId;
    std::string operationRevision;
    std::string timerAssignmentId;
    std::string expectedAssignmentRevision;
    std::string expectedIntentRevision;
    std::uint64_t assignmentEpoch = 0;
    std::string nativeTimerBindingId;
    std::string backendId;
    std::uint64_t backendGeneration = 0;
    BackendAgentNativeTimerCreateSpecification expectedSpecification;
    std::string expectedSpecificationFingerprint;
};

struct BackendAgentNativeTimerCreateReservationResult
{
    bool accepted = false;
    bool replayed = false;
    std::string reasonCode;
    BackendAgentCommandAssignment assignment;
};

class BackendAgentNativeTimerCreateReservationService
{
public:
    BackendAgentNativeTimerCreateReservationService(
        BackendAgentCommandRepository& commandRepository,
        BackendAgentCommandReservationRepository& reservationRepository,
        BackendAgentRepository& agentRepository);

    BackendAgentNativeTimerCreateReservationResult reserve(
        const RequestSecurityContext& context,
        const BackendAgentNativeTimerCreateReservationRequest& request,
        std::int64_t now,
        std::int64_t deadline);

private:
    BackendAgentCommandRepository& commandRepository_;
    BackendAgentCommandReservationRepository& reservationRepository_;
    BackendAgentRepository& agentRepository_;
};

} // namespace vdrsuite::agent
