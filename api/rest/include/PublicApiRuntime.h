#pragma once

#include "DashboardController.h"
#include "PublicTimerCreateRequestParser.h"

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

enum class PublicTimerCreateSubmissionStatus
{
    accepted,
    invalid,
    notFound,
    revisionConflict,
    idempotencyConflict,
    stateConflict,
    generationConflict,
    backendUnavailable,
    capabilityUnavailable,
    unavailable,
};

struct PublicTimerCreateSubmissionRequest
{
    std::string timerAssignmentId;
    std::string backendId;
    std::string actorRef;
    std::string idempotencyKey;
    std::string expectedResourceRevision;
    PublicTimerCreateSpecification specification;
    std::string requestId;
    std::string correlationId;
};

struct PublicTimerCreateSubmissionResult
{
    PublicTimerCreateSubmissionStatus status =
        PublicTimerCreateSubmissionStatus::unavailable;
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

    using TimerCreateSubmission =
        std::function<PublicTimerCreateSubmissionResult(
            const PublicTimerCreateSubmissionRequest& request)>;

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

    void registerTimerCreateSubmission(TimerCreateSubmission submission);
    void resetTimerCreateSubmission();
    bool timerCreateSubmissionConfigured() const;

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
        const std::string& body,
        const std::string& actorRef,
        const std::string& requestId,
        const std::string& correlationId,
        ApiResponse& response,
        const std::string& idempotencyKey = "",
        const std::string& ifMatch = "",
        const std::string& authorizedBackendId = "") const;

    bool tryHandlePost(
        const std::string& requestTarget,
        const std::string& requestId,
        const std::string& correlationId,
        ApiResponse& response) const
    {
        return tryHandlePost(
            requestTarget,
            "",
            "",
            requestId,
            correlationId,
            response);
    }

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

    PublicTimerCreateSubmissionResult submitTimerCreate(
        const PublicTimerCreateSubmissionRequest& request) const;

    mutable std::mutex operationLookupMutex_;
    OperationLookup operationLookup_;

    mutable std::mutex timerAssignmentLookupMutex_;
    TimerAssignmentLookup timerAssignmentLookup_;

    mutable std::mutex timerCreateSubmissionMutex_;
    TimerCreateSubmission timerCreateSubmission_;
};
