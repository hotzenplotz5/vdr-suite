#pragma once

#include "DashboardController.h"

#include <functional>
#include <mutex>
#include <string>

enum class PublicOperationLookupStatus
{
    ok,
    invalid,
    notFound,
    unavailable,
};

struct PublicOperationResource
{
    std::string operationId;
    std::string state;
    std::string backendId;
    std::string resourceRevision;
};

struct PublicOperationLookupResult
{
    PublicOperationLookupStatus status =
        PublicOperationLookupStatus::unavailable;
    PublicOperationResource operation;
};

enum class PublicTimerAssignmentLookupStatus
{
    ok,
    invalid,
    notFound,
    unavailable,
};

struct PublicTimerAssignmentRevisionResource
{
    std::string timerAssignmentId;
    std::string backendId;
    std::string resourceRevision;
};

struct PublicTimerAssignmentLookupResult
{
    PublicTimerAssignmentLookupStatus status =
        PublicTimerAssignmentLookupStatus::unavailable;
    PublicTimerAssignmentRevisionResource assignment;
};

enum class PublicTimerCreateAdmissionStatus
{
    accepted,
    replayed,
    invalid,
    notFound,
    readOnlyBackend,
    backendUnavailable,
    revisionConflict,
    stateConflict,
    generationConflict,
    idempotencyConflict,
    operationConflict,
    serviceUnavailable,
};

struct PublicTimerCreateAdmissionRequest
{
    std::string actorRef;
    std::string backendId;
    std::string timerAssignmentId;
    std::string expectedAssignmentRevision;
    std::string idempotencyKey;
};

struct PublicTimerCreateAdmissionResult
{
    PublicTimerCreateAdmissionStatus status =
        PublicTimerCreateAdmissionStatus::serviceUnavailable;
    PublicOperationResource operation;
};

class PublicApiRuntime
{
public:
    using OperationLookup =
        std::function<PublicOperationLookupResult(
            const std::string& operationId,
            const std::string& actorRef)>;

    using TimerAssignmentLookup =
        std::function<PublicTimerAssignmentLookupResult(
            const std::string& timerAssignmentId,
            const std::string& backendId)>;

    using TimerCreateAdmission =
        std::function<PublicTimerCreateAdmissionResult(
            const PublicTimerCreateAdmissionRequest& request)>;

    static PublicApiRuntime& instance();

    void registerOperationLookup(OperationLookup lookup);
    void resetOperationLookup();
    bool operationLookupConfigured() const;

    void registerTimerAssignmentLookup(TimerAssignmentLookup lookup);
    void resetTimerAssignmentLookup();
    bool timerAssignmentLookupConfigured() const;
    PublicTimerAssignmentLookupResult lookupTimerAssignment(
        const std::string& timerAssignmentId,
        const std::string& backendId) const;

    void registerTimerCreateAdmission(TimerCreateAdmission admission);
    void resetTimerCreateAdmission();
    bool timerCreateAdmissionConfigured() const;

    bool tryHandleGet(
        const std::string& requestTarget,
        const std::string& actorRef,
        const std::string& requestId,
        const std::string& correlationId,
        ApiResponse& response,
        const std::string& ifNoneMatch = "",
        const std::string& authorizedBackendId = "") const;

    bool tryHandlePost(
        const std::string& requestTarget,
        const std::string& requestId,
        const std::string& correlationId,
        ApiResponse& response,
        const std::string& body = "",
        const std::string& actorRef = "",
        const std::string& ifMatch = "",
        const std::string& idempotencyKey = "",
        const std::string& contentType = "",
        const std::string& authorizedBackendId = "") const;

    bool tryHandleUnsupportedMethod(
        const std::string& method,
        const std::string& requestTarget,
        const std::string& requestId,
        const std::string& correlationId,
        ApiResponse& response) const;

private:
    PublicApiRuntime() = default;

    PublicOperationLookupResult lookupOperation(
        const std::string& operationId,
        const std::string& actorRef) const;

    mutable std::mutex operationLookupMutex_;
    OperationLookup operationLookup_;

    mutable std::mutex timerAssignmentLookupMutex_;
    TimerAssignmentLookup timerAssignmentLookup_;

    mutable std::mutex timerCreateAdmissionMutex_;
    TimerCreateAdmission timerCreateAdmission_;
};
