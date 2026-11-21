#pragma once

#include "BackendAgentCommand.h"

#include <string>

class BackendAgentCommandRepository;
class BackendAgentCommandReservationRepository;

namespace vdrsuite::operations
{
class MutationOperationRepository;
}

namespace vdrsuite::agent
{

enum class BackendAgentNativeTimerCreateActivationStatus
{
    activated,
    alreadyActivated,
    operationNotFound,
    operationNotDispatching,
    dispatchReferenceInvalid,
    reservationNotFound,
    identityConflict,
    payloadConflict,
    providerSelectionStale,
    activationConflict,
    storageError,
    invalid,
};

struct BackendAgentNativeTimerCreateActivationResult
{
    BackendAgentNativeTimerCreateActivationStatus status =
        BackendAgentNativeTimerCreateActivationStatus::storageError;
    BackendAgentCommandAssignment assignment;
    std::string reasonCode;

    bool ok() const
    {
        return status == BackendAgentNativeTimerCreateActivationStatus::activated ||
            status == BackendAgentNativeTimerCreateActivationStatus::alreadyActivated;
    }
};

class BackendAgentNativeTimerCreateActivationService
{
public:
    BackendAgentNativeTimerCreateActivationService(
        vdrsuite::operations::MutationOperationRepository& operationRepository,
        BackendAgentCommandReservationRepository& reservationRepository,
        BackendAgentCommandRepository& commandRepository);

    BackendAgentNativeTimerCreateActivationResult activateDispatching(
        const std::string& operationId);

private:
    vdrsuite::operations::MutationOperationRepository& operationRepository_;
    BackendAgentCommandReservationRepository& reservationRepository_;
    BackendAgentCommandRepository& commandRepository_;
};

} // namespace vdrsuite::agent
