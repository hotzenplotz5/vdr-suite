#pragma once

#include "DashboardController.h"

#include <cstddef>
#include <functional>
#include <mutex>
#include <string>
#include <vector>

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

enum class PublicTimerAssignmentCollectionStatus
{
    ok,
    invalid,
    unavailable,
};

struct PublicTimerAssignmentCollectionItem
{
    std::string timerAssignmentId;
    std::string backendId;
};

struct PublicTimerAssignmentCollectionRequest
{
    std::string backendId;
    std::string afterTimerAssignmentId;
    std::size_t limit = 50U;
};

struct PublicTimerAssignmentCollectionResult
{
    PublicTimerAssignmentCollectionStatus status =
        PublicTimerAssignmentCollectionStatus::unavailable;
    std::vector<PublicTimerAssignmentCollectionItem> assignments;
    bool hasMore = false;
};

enum class PublicChannelCollectionStatus
{
    ok,
    invalid,
    allSourcesUnavailable,
    unavailable,
};

struct PublicChannelCollectionItem
{
    std::string backendId;
    std::string channelId;
    int channelNumber = 0;
    std::string name;
    std::string provider;
    std::string groupName;
    bool radio = false;
    bool encrypted = false;
    bool enabled = false;
};

struct PublicChannelCollectionSource
{
    std::string backendId;
    std::string state;
    std::string code;
};

struct PublicChannelCollectionRequest
{
    std::vector<std::string> backendIds;
    std::string afterBackendId;
    std::string afterChannelId;
    std::size_t limit = 50U;
};

struct PublicChannelCollectionResult
{
    PublicChannelCollectionStatus status =
        PublicChannelCollectionStatus::unavailable;
    std::vector<PublicChannelCollectionItem> channels;
    std::vector<PublicChannelCollectionSource> sources;
    bool hasMore = false;
};

enum class PublicBackendCollectionStatus
{
    ok,
    unavailable,
};

struct PublicBackendCollectionItem
{
    std::string backendId;
    std::string name;
    std::string type;
    bool enabled = false;
    bool online = false;
};

struct PublicBackendCollectionRequest
{
    std::vector<std::string> authorizedBackendIds;
    std::string afterBackendId;
    std::size_t limit = 50U;
};

struct PublicBackendCollectionResult
{
    PublicBackendCollectionStatus status =
        PublicBackendCollectionStatus::unavailable;
    std::vector<PublicBackendCollectionItem> backends;
    bool hasMore = false;
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

    using TimerAssignmentCollectionLookup =
        std::function<PublicTimerAssignmentCollectionResult(
            const PublicTimerAssignmentCollectionRequest& request)>;

    using ChannelCollectionLookup =
        std::function<PublicChannelCollectionResult(
            const PublicChannelCollectionRequest& request)>;

    using BackendCollectionLookup =
        std::function<PublicBackendCollectionResult(
            const PublicBackendCollectionRequest& request)>;

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

    void registerTimerAssignmentCollectionLookup(
        TimerAssignmentCollectionLookup lookup);
    void resetTimerAssignmentCollectionLookup();
    bool timerAssignmentCollectionLookupConfigured() const;
    PublicTimerAssignmentCollectionResult lookupTimerAssignmentCollection(
        const PublicTimerAssignmentCollectionRequest& request) const;

    void registerChannelCollectionLookup(
        ChannelCollectionLookup lookup);
    void resetChannelCollectionLookup();
    bool channelCollectionLookupConfigured() const;
    PublicChannelCollectionResult lookupChannelCollection(
        const PublicChannelCollectionRequest& request) const;

    void registerBackendCollectionLookup(
        BackendCollectionLookup lookup);
    void resetBackendCollectionLookup();
    bool backendCollectionLookupConfigured() const;
    PublicBackendCollectionResult lookupBackendCollection(
        const PublicBackendCollectionRequest& request) const;

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
        const std::string& authorizedBackendId = "",
        const std::vector<std::string>& authorizedBackendIds = {}) const;

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

    mutable std::mutex timerAssignmentCollectionLookupMutex_;
    TimerAssignmentCollectionLookup timerAssignmentCollectionLookup_;

    mutable std::mutex channelCollectionLookupMutex_;
    ChannelCollectionLookup channelCollectionLookup_;

    mutable std::mutex backendCollectionLookupMutex_;
    BackendCollectionLookup backendCollectionLookup_;

    mutable std::mutex timerCreateAdmissionMutex_;
    TimerCreateAdmission timerCreateAdmission_;
};
