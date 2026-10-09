#pragma once

#include "DashboardController.h"

#include <cstddef>
#include <cstdint>
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

enum class PublicRecordingCollectionStatus
{
    ok,
    invalid,
    unavailable,
};

struct PublicRecordingCollectionItem
{
    std::string recordingId;
    std::string backendId;
    std::string title;
    std::string recordedAt;
    int durationSeconds = 0;
    bool durationKnown = false;
    // Read-only descriptive facts; no artwork/provider URL or native path.
    std::string description;
    long long sizeMb = 0;
};

struct PublicRecordingFolderItem
{
    std::string folderId;
    std::string name;
    int recordingCount = 0;
};

struct PublicRecordingCollectionRequest
{
    std::string backendId;
    std::string afterRecordingId;
    std::size_t limit = 50U;
    bool browseFolders = false;
    std::string folderId;
    std::size_t offset = 0U;
};

struct PublicRecordingCollectionResult
{
    PublicRecordingCollectionStatus status =
        PublicRecordingCollectionStatus::unavailable;
    std::vector<PublicRecordingCollectionItem> recordings;
    std::vector<PublicRecordingFolderItem> folders;
    std::size_t totalEntries = 0U;
    bool hasMore = false;
};

enum class PublicGenreCollectionStatus
{
    ok,
    invalid,
    unavailable,
};

struct PublicGenreCollectionRequest
{
    std::string backendId;
    std::string genreId;
    std::string locale = "de";
    std::size_t limit = 30U;
    std::size_t offset = 0U;
    bool recordings = false;
};

struct PublicGenreItem
{
    std::string genreId;
    std::string label;
    std::string labelDe;
    std::string labelEn;
    std::size_t count = 0U;
};

struct PublicGenreRecordingItem
{
    std::string recordingId;
    std::string backendId;
    std::string title;
    std::string recordedAt;
    int durationSeconds = 0;
    bool durationKnown = false;
};

struct PublicGenreCollectionResult
{
    PublicGenreCollectionStatus status = PublicGenreCollectionStatus::unavailable;
    std::vector<PublicGenreItem> genres;
    std::vector<PublicGenreRecordingItem> recordings;
    std::size_t totalCount = 0U;
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

enum class PublicEpgNowNextStatus
{
    ok,
    invalid,
    notFound,
    unavailable,
};

// One authorized backend and one real VDR channel; never an arbitrary native URL.
struct PublicEpgNowNextRequest
{
    std::string backendId;
    std::string channelId;
    std::string fromTime;
    std::size_t limit = 2U;
};

struct PublicEpgNowNextItem
{
    std::string channelId;
    std::string title;
    std::string subtitle;
    std::string startTime;
    std::string endTime;
    int durationSeconds = 0;
};

struct PublicEpgNowNextResult
{
    PublicEpgNowNextStatus status = PublicEpgNowNextStatus::unavailable;
    std::vector<PublicEpgNowNextItem> events;
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

enum class PublicAccountCollectionStatus
{
    ok,
    unavailable,
};

struct PublicAccountCollectionItem
{
    std::string accountId;
    std::string actorId;
    std::string displayName;
    bool active = false;
};

struct PublicAccountCollectionRequest
{
    std::string afterAccountId;
    std::size_t limit = 50U;
};

struct PublicAccountCollectionResult
{
    PublicAccountCollectionStatus status =
        PublicAccountCollectionStatus::unavailable;
    std::vector<PublicAccountCollectionItem> accounts;
    bool hasMore = false;
};

enum class PublicAccountLookupStatus
{
    ok,
    invalid,
    notFound,
    unavailable,
};

struct PublicAccountResource
{
    std::string accountId;
    std::string actorId;
    std::string displayName;
    bool active = false;
    std::string resourceRevision;
};

struct PublicAccountLookupResult
{
    PublicAccountLookupStatus status =
        PublicAccountLookupStatus::unavailable;
    PublicAccountResource account;
};

enum class PublicAccountMutationKind
{
    displayName,
    active,
};

enum class PublicAccountMutationStatus
{
    ok,
    invalid,
    notFound,
    revisionConflict,
    finalAdministrator,
    unavailable,
};

struct PublicAccountMutationRequest
{
    std::string actorRef;
    std::string accountId;
    std::uint64_t expectedRevision = 0;
    PublicAccountMutationKind kind =
        PublicAccountMutationKind::displayName;
    std::string displayName;
    bool active = false;
    std::string requestId;
    std::string correlationId;
};

struct PublicAccountMutationResult
{
    PublicAccountMutationStatus status =
        PublicAccountMutationStatus::unavailable;
    PublicAccountResource account;
    std::size_t revokedBrowserSessions = 0;
};

enum class PublicAccountCreateStatus
{
    created,
    replayed,
    invalid,
    loginConflict,
    idempotencyConflict,
    unavailable,
};

struct PublicAccountCreateRequest
{
    std::string actorRef;
    std::string loginName;
    std::string displayName;
    std::string password;
    std::string idempotencyKey;
    std::string requestId;
    std::string correlationId;
};

struct PublicAccountCreateResult
{
    PublicAccountCreateStatus status =
        PublicAccountCreateStatus::unavailable;
    PublicAccountResource account;
};

enum class PublicAccountGrantStatus
{
    ok,
    invalid,
    notFound,
    revisionConflict,
    finalAdministrator,
    unavailable,
};

struct PublicAccountGrantItem
{
    std::string permission;
    std::string backendId;
};

struct PublicAccountGrantOption
{
    std::string permission;
    std::string presentationKey;
    std::string category;
};

struct PublicAccountGrantSetResource
{
    std::string accountId;
    std::string actorId;
    std::vector<PublicAccountGrantItem> grants;
    std::vector<std::string> supportedPermissions;
    std::vector<PublicAccountGrantOption> supportedPermissionOptions;
    std::vector<std::string> supportedScopeKinds;
    std::string resourceRevision;
};

struct PublicAccountGrantLookupResult
{
    PublicAccountGrantStatus status =
        PublicAccountGrantStatus::unavailable;
    PublicAccountGrantSetResource grantSet;
};

struct PublicAccountGrantMutationRequest
{
    std::string actorRef;
    std::string accountId;
    std::string expectedResourceRevision;
    std::string permission;
    std::string backendId;
    bool active = false;
    std::string requestId;
    std::string correlationId;
};

struct PublicAccountGrantMutationResult
{
    PublicAccountGrantStatus status =
        PublicAccountGrantStatus::unavailable;
    PublicAccountGrantSetResource grantSet;
};

enum class PublicDeviceLifecycleStatus
{
    ok, invalid, notFound, revisionConflict, unavailable
};

struct PublicDeviceLifecycleResource
{
    std::string deviceId;
    std::string actorId;
    std::string credentialId;
    bool active = false;
    bool revoked = false;
    std::string resourceRevision;
};

struct PublicDeviceLifecycleResult
{
    PublicDeviceLifecycleStatus status =
        PublicDeviceLifecycleStatus::unavailable;
    PublicDeviceLifecycleResource resource;
};

struct PublicDeviceLifecycleMutationRequest
{
    std::string actorRef;
    std::string deviceId;
    std::string credentialId;
    std::string expectedResourceRevision;
    std::string requestId;
    std::string correlationId;
};

enum class PublicDeviceCredentialRotationStatus
{
    rotated, invalid, notFound, stateConflict, revisionConflict, unavailable
};

struct PublicDeviceCredentialRotationRequest
{
    std::string actorRef;
    std::string deviceId;
    std::string credentialId;
    std::string expectedResourceRevision;
    std::string requestId;
    std::string correlationId;
};

struct PublicDeviceCredentialRotationResult
{
    PublicDeviceCredentialRotationStatus status =
        PublicDeviceCredentialRotationStatus::unavailable;
    std::string actorId;
    std::string deviceId;
    std::string credentialId;
    std::string credentialSecret;
};

enum class PublicDeviceGrantStatus
{
    ok, invalid, notFound, revisionConflict, unavailable
};

struct PublicDeviceGrantSetResource
{
    std::string deviceId;
    std::string actorId;
    std::vector<PublicAccountGrantItem> grants;
    std::string resourceRevision;
};

struct PublicDeviceGrantLookupResult
{
    PublicDeviceGrantStatus status = PublicDeviceGrantStatus::unavailable;
    PublicDeviceGrantSetResource grantSet;
};

struct PublicDeviceGrantMutationRequest
{
    std::string actorRef;
    std::string deviceId;
    std::string expectedResourceRevision;
    std::string permission;
    std::string backendId;
    bool active = false;
    std::string requestId;
    std::string correlationId;
};

struct PublicDeviceGrantMutationResult
{
    PublicDeviceGrantStatus status = PublicDeviceGrantStatus::unavailable;
    PublicDeviceGrantSetResource grantSet;
};

enum class PublicAccountSecurityMetadataStatus
{
    ok,
    invalid,
    notFound,
    unavailable,
};

struct PublicAccountCredentialItem
{
    std::string credentialId;
    std::string credentialType;
    bool active = false;
    bool expired = false;
    bool revoked = false;
    std::string expiresAt;
    std::string createdAt;
};

struct PublicAccountCredentialCollectionResource
{
    std::string accountId;
    std::string actorId;
    std::vector<PublicAccountCredentialItem> credentials;
};

struct PublicAccountCredentialCollectionResult
{
    PublicAccountSecurityMetadataStatus status =
        PublicAccountSecurityMetadataStatus::unavailable;
    PublicAccountCredentialCollectionResource collection;
};

enum class PublicAccountCredentialAdministrationStatus
{
    ok,
    invalid,
    notFound,
    validationError,
    revisionConflict,
    finalAdministrator,
    unavailable,
};

struct PublicAccountCredentialResource
{
    std::string accountId;
    std::string actorId;
    PublicAccountCredentialItem credential;
    std::string resourceRevision;
};

struct PublicAccountCredentialLookupResult
{
    PublicAccountCredentialAdministrationStatus status =
        PublicAccountCredentialAdministrationStatus::unavailable;
    PublicAccountCredentialResource resource;
};

struct PublicAccountCredentialMutationRequest
{
    std::string actorRef;
    std::string accountId;
    std::string credentialId;
    std::string expectedResourceRevision;
    std::string requestId;
    std::string correlationId;
};

struct PublicAccountCredentialMutationResult
{
    PublicAccountCredentialAdministrationStatus status =
        PublicAccountCredentialAdministrationStatus::unavailable;
    PublicAccountCredentialResource resource;
};

struct PublicAccountSessionItem
{
    std::string sessionId;
    std::string deviceId;
    std::string issuedFromCredentialId;
    bool active = false;
    bool expired = false;
    bool revoked = false;
    std::string expiresAt;
    std::string lastSeenAt;
    std::string createdAt;
};

struct PublicAccountSessionCollectionResource
{
    std::string accountId;
    std::string actorId;
    std::vector<PublicAccountSessionItem> sessions;
};

struct PublicAccountSessionCollectionResult
{
    PublicAccountSecurityMetadataStatus status =
        PublicAccountSecurityMetadataStatus::unavailable;
    PublicAccountSessionCollectionResource collection;
};

enum class PublicAccountSessionAdministrationStatus
{
    ok,
    invalid,
    notFound,
    revisionConflict,
    unavailable,
};

struct PublicAccountSessionResource
{
    std::string accountId;
    std::string actorId;
    PublicAccountSessionItem session;
    std::string resourceRevision;
};

struct PublicAccountSessionLookupResult
{
    PublicAccountSessionAdministrationStatus status =
        PublicAccountSessionAdministrationStatus::unavailable;
    PublicAccountSessionResource resource;
};

struct PublicAccountSessionMutationRequest
{
    std::string actorRef;
    std::string accountId;
    std::string sessionId;
    std::string expectedResourceRevision;
    std::string requestId;
    std::string correlationId;
};

struct PublicAccountSessionMutationResult
{
    PublicAccountSessionAdministrationStatus status =
        PublicAccountSessionAdministrationStatus::unavailable;
    PublicAccountSessionResource resource;
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

enum class PublicDevicePairingCreateStatus
{
    created,
    invalid,
    entropyUnavailable,
    hashingUnavailable,
    unavailable,
};

enum class PublicDevicePairingLookupStatus
{
    ok,
    invalid,
    notFound,
    unauthorized,
    expired,
    consumed,
    unavailable,
};

struct PublicDevicePairingClientMetadata
{
    std::string displayName;
    std::string clientKind;
    std::string appVersion;
};

struct PublicDevicePairingResource
{
    std::string pairingRequestId;
    PublicDevicePairingClientMetadata client;
    std::string state;
    std::string expiresAt;
    int pollIntervalSeconds = 0;
};

struct PublicDevicePairingCreateRequest
{
    PublicDevicePairingClientMetadata client;
    std::string requestId;
    std::string correlationId;
};

struct PublicDevicePairingCreateResult
{
    PublicDevicePairingCreateStatus status =
        PublicDevicePairingCreateStatus::unavailable;
    PublicDevicePairingResource resource;
    std::string userCode;
    std::string pairingToken;
};

struct PublicDevicePairingLookupRequest
{
    std::string pairingRequestId;
    std::string pairingToken;
};

struct PublicDevicePairingLookupResult
{
    PublicDevicePairingLookupStatus status =
        PublicDevicePairingLookupStatus::unavailable;
    PublicDevicePairingResource resource;
};

enum class PublicDeviceCredentialIssueStatus
{
    issued,
    invalid,
    notFound,
    unauthorized,
    notApproved,
    expired,
    consumed,
    unavailable,
};

struct PublicDeviceCredentialIssueRequest
{
    std::string pairingRequestId;
    std::string pairingToken;
    std::string requestId;
    std::string correlationId;
};

struct PublicDeviceCredentialIssueResult
{
    PublicDeviceCredentialIssueStatus status =
        PublicDeviceCredentialIssueStatus::unavailable;
    std::string actorId;
    std::string deviceId;
    std::string credentialId;
    std::string credentialSecret;
};

enum class PublicDevicePairingAdministrationStatus
{
    ok,
    invalid,
    notFound,
    expired,
    revisionConflict,
    stateConflict,
    unavailable,
};

struct PublicDevicePairingAdministrativeResource
{
    PublicDevicePairingResource resource;
    std::string resourceRevision;
    std::string decidedByActorId;
    std::string decidedAt;
};

struct PublicDevicePairingAdministrationCollectionRequest
{
    std::string afterPairingRequestId;
    std::size_t limit = 0U;
};

struct PublicDevicePairingAdministrationCollectionResult
{
    PublicDevicePairingAdministrationStatus status =
        PublicDevicePairingAdministrationStatus::unavailable;
    std::vector<PublicDevicePairingAdministrativeResource> requests;
    bool hasMore = false;
};

struct PublicDevicePairingAdministrationLookupResult
{
    PublicDevicePairingAdministrationStatus status =
        PublicDevicePairingAdministrationStatus::unavailable;
    PublicDevicePairingAdministrativeResource request;
};

struct PublicDevicePairingDecisionRequest
{
    std::string actorRef;
    std::string pairingRequestId;
    std::string expectedResourceRevision;
    std::string decision;
    std::string requestId;
    std::string correlationId;
};

struct PublicDevicePairingDecisionResult
{
    PublicDevicePairingAdministrationStatus status =
        PublicDevicePairingAdministrationStatus::unavailable;
    PublicDevicePairingAdministrativeResource request;
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

    using RecordingCollectionLookup =
        std::function<PublicRecordingCollectionResult(
            const PublicRecordingCollectionRequest& request)>;

    using GenreCollectionLookup =
        std::function<PublicGenreCollectionResult(
            const PublicGenreCollectionRequest& request)>;

    using ChannelCollectionLookup =
        std::function<PublicChannelCollectionResult(
            const PublicChannelCollectionRequest& request)>;

    using EpgNowNextLookup =
        std::function<PublicEpgNowNextResult(
            const PublicEpgNowNextRequest& request)>;

    using BackendCollectionLookup =
        std::function<PublicBackendCollectionResult(
            const PublicBackendCollectionRequest& request)>;

    using AccountCollectionLookup =
        std::function<PublicAccountCollectionResult(
            const PublicAccountCollectionRequest& request)>;

    using AccountLookup =
        std::function<PublicAccountLookupResult(
            const std::string& accountId)>;

    using AccountMutation =
        std::function<PublicAccountMutationResult(
            const PublicAccountMutationRequest& request)>;

    using AccountCreate =
        std::function<PublicAccountCreateResult(
            const PublicAccountCreateRequest& request)>;

    using DeviceLifecycleLookup =
        std::function<PublicDeviceLifecycleResult(
            const std::string& deviceId,
            const std::string& credentialId)>;
    using DeviceLifecycleMutation =
        std::function<PublicDeviceLifecycleResult(
            const PublicDeviceLifecycleMutationRequest& request)>;
    using DeviceCredentialRotation =
        std::function<PublicDeviceCredentialRotationResult(
            const PublicDeviceCredentialRotationRequest& request)>;

    using DeviceGrantLookup =
        std::function<PublicDeviceGrantLookupResult(
            const std::string& deviceId)>;
    using DeviceGrantMutation =
        std::function<PublicDeviceGrantMutationResult(
            const PublicDeviceGrantMutationRequest& request)>;

    using AccountGrantLookup =
        std::function<PublicAccountGrantLookupResult(
            const std::string& accountId)>;

    using AccountGrantMutation =
        std::function<PublicAccountGrantMutationResult(
            const PublicAccountGrantMutationRequest& request)>;

    using AccountCredentialLookup =
        std::function<PublicAccountCredentialCollectionResult(
            const std::string& accountId)>;

    using AccountCredentialItemLookup =
        std::function<PublicAccountCredentialLookupResult(
            const std::string& accountId,
            const std::string& credentialId)>;

    using AccountCredentialMutation =
        std::function<PublicAccountCredentialMutationResult(
            const PublicAccountCredentialMutationRequest& request)>;

    using AccountSessionLookup =
        std::function<PublicAccountSessionCollectionResult(
            const std::string& accountId)>;

    using AccountSessionItemLookup =
        std::function<PublicAccountSessionLookupResult(
            const std::string& accountId,
            const std::string& sessionId)>;

    using AccountSessionMutation =
        std::function<PublicAccountSessionMutationResult(
            const PublicAccountSessionMutationRequest& request)>;

    using DevicePairingCreate =
        std::function<PublicDevicePairingCreateResult(
            const PublicDevicePairingCreateRequest& request)>;

    using DevicePairingLookup =
        std::function<PublicDevicePairingLookupResult(
            const PublicDevicePairingLookupRequest& request)>;

    using DeviceCredentialIssue =
        std::function<PublicDeviceCredentialIssueResult(
            const PublicDeviceCredentialIssueRequest& request)>;

    using DevicePairingAdministrationCollectionLookup =
        std::function<PublicDevicePairingAdministrationCollectionResult(
            const PublicDevicePairingAdministrationCollectionRequest& request)>;

    using DevicePairingAdministrationLookup =
        std::function<PublicDevicePairingAdministrationLookupResult(
            const std::string& pairingRequestId)>;

    using DevicePairingDecision =
        std::function<PublicDevicePairingDecisionResult(
            const PublicDevicePairingDecisionRequest& request)>;

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

    void registerRecordingCollectionLookup(RecordingCollectionLookup lookup);
    void resetRecordingCollectionLookup();
    bool recordingCollectionLookupConfigured() const;
    PublicRecordingCollectionResult lookupRecordingCollection(
        const PublicRecordingCollectionRequest& request) const;

    void registerGenreCollectionLookup(GenreCollectionLookup lookup);
    void resetGenreCollectionLookup();
    bool genreCollectionLookupConfigured() const;
    PublicGenreCollectionResult lookupGenreCollection(
        const PublicGenreCollectionRequest& request) const;

    void registerChannelCollectionLookup(
        ChannelCollectionLookup lookup);
    void resetChannelCollectionLookup();
    bool channelCollectionLookupConfigured() const;
    PublicChannelCollectionResult lookupChannelCollection(
        const PublicChannelCollectionRequest& request) const;

    void registerEpgNowNextLookup(EpgNowNextLookup lookup);
    void resetEpgNowNextLookup();
    bool epgNowNextLookupConfigured() const;
    PublicEpgNowNextResult lookupEpgNowNext(
        const PublicEpgNowNextRequest& request) const;

    void registerBackendCollectionLookup(
        BackendCollectionLookup lookup);
    void resetBackendCollectionLookup();
    bool backendCollectionLookupConfigured() const;
    PublicBackendCollectionResult lookupBackendCollection(
        const PublicBackendCollectionRequest& request) const;

    void registerAccountCollectionLookup(
        AccountCollectionLookup lookup);
    void resetAccountCollectionLookup();
    bool accountCollectionLookupConfigured() const;
    PublicAccountCollectionResult lookupAccountCollection(
        const PublicAccountCollectionRequest& request) const;

    void registerAccountLookup(AccountLookup lookup);
    void resetAccountLookup();
    bool accountLookupConfigured() const;
    PublicAccountLookupResult lookupAccount(
        const std::string& accountId) const;

    void registerAccountMutation(AccountMutation mutation);
    void resetAccountMutation();
    bool accountMutationConfigured() const;

    void registerAccountCreate(AccountCreate create);
    void resetAccountCreate();
    bool accountCreateConfigured() const;

    void registerDeviceLifecycleLookup(DeviceLifecycleLookup lookup);
    void resetDeviceLifecycleLookup();
    void registerDeviceLifecycleMutation(DeviceLifecycleMutation mutation);
    void resetDeviceLifecycleMutation();
    bool deviceLifecycleAdministrationConfigured() const;
    void registerDeviceCredentialRotation(DeviceCredentialRotation rotate);
    void resetDeviceCredentialRotation();
    bool deviceCredentialRotationConfigured() const;

    void registerDeviceGrantLookup(DeviceGrantLookup lookup);
    void resetDeviceGrantLookup();
    void registerDeviceGrantMutation(DeviceGrantMutation mutation);
    void resetDeviceGrantMutation();
    bool deviceGrantAdministrationConfigured() const;

    void registerAccountGrantLookup(
        AccountGrantLookup lookup);
    void resetAccountGrantLookup();
    bool accountGrantLookupConfigured() const;

    void registerAccountGrantMutation(
        AccountGrantMutation mutation);
    void resetAccountGrantMutation();
    bool accountGrantMutationConfigured() const;

    void registerAccountCredentialLookup(
        AccountCredentialLookup lookup);
    void resetAccountCredentialLookup();
    bool accountCredentialLookupConfigured() const;

    void registerAccountCredentialItemLookup(
        AccountCredentialItemLookup lookup);
    void resetAccountCredentialItemLookup();
    bool accountCredentialItemLookupConfigured() const;

    void registerAccountCredentialMutation(
        AccountCredentialMutation mutation);
    void resetAccountCredentialMutation();
    bool accountCredentialMutationConfigured() const;

    void registerAccountSessionLookup(
        AccountSessionLookup lookup);
    void resetAccountSessionLookup();
    bool accountSessionLookupConfigured() const;

    void registerAccountSessionItemLookup(
        AccountSessionItemLookup lookup);
    void resetAccountSessionItemLookup();
    bool accountSessionItemLookupConfigured() const;

    void registerAccountSessionMutation(
        AccountSessionMutation mutation);
    void resetAccountSessionMutation();
    bool accountSessionMutationConfigured() const;

    void registerDevicePairingCreate(
        DevicePairingCreate create);
    void resetDevicePairingCreate();
    bool devicePairingCreateConfigured() const;

    void registerDevicePairingLookup(
        DevicePairingLookup lookup);
    void resetDevicePairingLookup();
    bool devicePairingLookupConfigured() const;

    void registerDeviceCredentialIssue(DeviceCredentialIssue issue);
    void resetDeviceCredentialIssue();
    bool deviceCredentialIssueConfigured() const;

    void registerDevicePairingAdministrationCollectionLookup(
        DevicePairingAdministrationCollectionLookup lookup);
    void resetDevicePairingAdministrationCollectionLookup();
    bool devicePairingAdministrationCollectionLookupConfigured() const;

    void registerDevicePairingAdministrationLookup(
        DevicePairingAdministrationLookup lookup);
    void resetDevicePairingAdministrationLookup();
    bool devicePairingAdministrationLookupConfigured() const;

    void registerDevicePairingDecision(
        DevicePairingDecision decision);
    void resetDevicePairingDecision();
    bool devicePairingDecisionConfigured() const;

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
        const std::vector<std::string>& authorizedBackendIds = {},
        const std::string& pairingToken = "") const;

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
        const std::string& authorizedBackendId = "",
        const std::string& pairingToken = "") const;

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

    mutable std::mutex recordingCollectionLookupMutex_;
    RecordingCollectionLookup recordingCollectionLookup_;

    mutable std::mutex genreCollectionLookupMutex_;
    GenreCollectionLookup genreCollectionLookup_;

    mutable std::mutex channelCollectionLookupMutex_;
    ChannelCollectionLookup channelCollectionLookup_;

    mutable std::mutex epgNowNextLookupMutex_;
    EpgNowNextLookup epgNowNextLookup_;

    mutable std::mutex backendCollectionLookupMutex_;
    BackendCollectionLookup backendCollectionLookup_;

    mutable std::mutex accountCollectionLookupMutex_;
    AccountCollectionLookup accountCollectionLookup_;

    mutable std::mutex accountLookupMutex_;
    AccountLookup accountLookup_;

    mutable std::mutex accountMutationMutex_;
    AccountMutation accountMutation_;

    mutable std::mutex accountCreateMutex_;
    AccountCreate accountCreate_;

    mutable std::mutex deviceLifecycleLookupMutex_;
    DeviceLifecycleLookup deviceLifecycleLookup_;

    mutable std::mutex deviceLifecycleMutationMutex_;
    DeviceLifecycleMutation deviceLifecycleMutation_;
    mutable std::mutex deviceCredentialRotationMutex_;
    DeviceCredentialRotation deviceCredentialRotation_;

    mutable std::mutex deviceGrantLookupMutex_;
    DeviceGrantLookup deviceGrantLookup_;

    mutable std::mutex deviceGrantMutationMutex_;
    DeviceGrantMutation deviceGrantMutation_;

    mutable std::mutex accountGrantLookupMutex_;
    AccountGrantLookup accountGrantLookup_;

    mutable std::mutex accountGrantMutationMutex_;
    AccountGrantMutation accountGrantMutation_;

    mutable std::mutex accountCredentialLookupMutex_;
    AccountCredentialLookup accountCredentialLookup_;

    mutable std::mutex accountCredentialItemLookupMutex_;
    AccountCredentialItemLookup accountCredentialItemLookup_;

    mutable std::mutex accountCredentialMutationMutex_;
    AccountCredentialMutation accountCredentialMutation_;

    mutable std::mutex accountSessionLookupMutex_;
    AccountSessionLookup accountSessionLookup_;

    mutable std::mutex accountSessionItemLookupMutex_;
    AccountSessionItemLookup accountSessionItemLookup_;

    mutable std::mutex accountSessionMutationMutex_;
    AccountSessionMutation accountSessionMutation_;

    mutable std::mutex devicePairingCreateMutex_;
    DevicePairingCreate devicePairingCreate_;

    mutable std::mutex devicePairingLookupMutex_;
    DevicePairingLookup devicePairingLookup_;

    mutable std::mutex deviceCredentialIssueMutex_;
    DeviceCredentialIssue deviceCredentialIssue_;

    mutable std::mutex devicePairingAdministrationCollectionLookupMutex_;
    DevicePairingAdministrationCollectionLookup
        devicePairingAdministrationCollectionLookup_;

    mutable std::mutex devicePairingAdministrationLookupMutex_;
    DevicePairingAdministrationLookup
        devicePairingAdministrationLookup_;

    mutable std::mutex devicePairingDecisionMutex_;
    DevicePairingDecision devicePairingDecision_;

    mutable std::mutex timerCreateAdmissionMutex_;
    TimerCreateAdmission timerCreateAdmission_;
};
