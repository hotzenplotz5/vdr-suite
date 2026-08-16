#pragma once

#include "BackendAgentCommand.h"
#include "BackendAgentLocalProvider.h"

#include <optional>
#include <string>

class BackendAgentCommandRepository;
class Database;

enum class BackendAgentCommandReservationStatus
{
    reserved,
    alreadyReserved,
    activated,
    alreadyActivated,
    notFound,
    conflict,
    storageError,
    invalid,
};

struct BackendAgentCommandReservation
{
    BackendAgentCommandAssignment assignment;
    bool localProviderSelectionPresent = false;
    vdrsuite::agent::BackendAgentLocalProviderSelection localProviderSelection;
    std::string assignmentFingerprint;
    std::string localProviderSelectionIdentity;
};

struct BackendAgentCommandReservationResult
{
    BackendAgentCommandReservationStatus status =
        BackendAgentCommandReservationStatus::storageError;
    BackendAgentCommandReservation reservation;

    bool ok() const
    {
        return status == BackendAgentCommandReservationStatus::reserved
            || status == BackendAgentCommandReservationStatus::alreadyReserved;
    }
};

struct BackendAgentCommandActivationResult
{
    BackendAgentCommandReservationStatus status =
        BackendAgentCommandReservationStatus::storageError;
    BackendAgentCommandAssignment assignment;

    bool ok() const
    {
        return status == BackendAgentCommandReservationStatus::activated
            || status == BackendAgentCommandReservationStatus::alreadyActivated;
    }
};

class BackendAgentCommandReservationRepository
{
public:
    explicit BackendAgentCommandReservationRepository(Database& database);

    bool ensureSchema();

    BackendAgentCommandReservationResult reserve(
        const BackendAgentCommandAssignment& assignment,
        const vdrsuite::agent::BackendAgentLocalProviderSelection* selection = nullptr);

    BackendAgentCommandReservationResult findByCommandId(
        const std::string& commandId) const;

    BackendAgentCommandReservationResult findForOperation(
        const std::string& backendId,
        const std::string& operationId,
        const std::string& commandType) const;

private:
    Database& database_;
};

class BackendAgentCommandReservationActivationService
{
public:
    BackendAgentCommandReservationActivationService(
        BackendAgentCommandReservationRepository& reservationRepository,
        BackendAgentCommandRepository& commandRepository);

    BackendAgentCommandActivationResult activate(
        const std::string& commandId);

private:
    BackendAgentCommandReservationRepository& reservationRepository_;
    BackendAgentCommandRepository& commandRepository_;
};
