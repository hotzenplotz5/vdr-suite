#pragma once

#include "EpgTimerSelectionGuard.h"
#include <cstdint>

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
    // Optional, journal-backed Agent result after independently verified
    // media import. Absent for all other operation families.
    std::string sourceCleanup = {};
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

    // Optional desired schedule presentation: not an actual native timer
    // observation or guarantee that recording/provisioning succeeded.
    bool plannedPresent = false;
    std::string plannedTitle;
    std::string plannedDay;
    std::string plannedWeekdays;
    std::string plannedStartTime;
    std::string plannedEndTime;
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

enum class PublicSearchTimerCollectionStatus
{
    ok,
    invalid,
    unavailable
};

struct PublicSearchTimerCollectionItem
{
    std::string searchTimerId;
    std::string backendId;
    std::string name;
    std::string query;
    std::string state;
};

struct PublicSearchTimerCollectionRequest
{
    std::string backendId;
    std::string afterSearchTimerId;
    std::size_t limit = 30U;
    // Optional exact opaque ID for a single saved-rule lookup.
    std::string searchTimerId;
};

struct PublicSearchTimerCollectionResult
{
    PublicSearchTimerCollectionStatus status =
        PublicSearchTimerCollectionStatus::unavailable;
    std::vector<PublicSearchTimerCollectionItem> items;
    bool hasMore = false;
};

enum class PublicSearchTimerCreateStatus
{
    accepted, replayed, invalid, conflict, backendUnavailable, unavailable
};
struct PublicSearchTimerCreateRequest
{
    std::string actorRef;
    std::string backendId;
    std::string idempotencyKey;
    std::string name;
    std::string query;
    // Safe creation staging: public-v1 rules are created inactive only.
    // Activation requires a separately fenced, future mutation workflow.
    bool active = false;
    int mode = 0;
    int tolerance = 0;
    bool compareTitle = true;
    bool compareSubtitle = false;
    bool compareSummary = false;
};
struct PublicSearchTimerCreateResult
{
    PublicSearchTimerCreateStatus status =
        PublicSearchTimerCreateStatus::unavailable;
    PublicOperationResource operation;
};

enum class PublicSearchTimerNativeAction
{
    update, activate, deactivate, remove
};
enum class PublicSearchTimerNativeVersionStatus
{
    ok, notFound, unavailable
};
struct PublicSearchTimerNativeVersionRequest
{
    std::string backendId;
    std::string publicSearchTimerId;
};
struct PublicSearchTimerNativeVersionResult
{
    PublicSearchTimerNativeVersionStatus status =
        PublicSearchTimerNativeVersionStatus::unavailable;
    std::string revision;
};
enum class PublicSearchTimerNativeMutationStatus
{
    accepted, replayed, invalid, conflict, notFound, unavailable
};
struct PublicSearchTimerNativeMutationRequest
{
    PublicSearchTimerNativeAction action =
        PublicSearchTimerNativeAction::update;
    std::string actorRef;
    std::string backendId;
    std::string publicSearchTimerId;
    std::string ifMatch;
    std::string idempotencyKey;
    std::string query;
    bool confirmed = false;
};
struct PublicSearchTimerNativeMutationResult
{
    PublicSearchTimerNativeMutationStatus status =
        PublicSearchTimerNativeMutationStatus::unavailable;
    PublicOperationResource operation;
};

// Read-only, bounded projection from Suite's existing SearchTimer preview.
enum class PublicSearchTimerPreviewStatus { ok, notFound, unavailable };

struct PublicSearchTimerPreviewEvent
{
    std::string title;
    std::string subtitle;
    std::string channelId;
    std::string startTime;
    std::string endTime;
};

struct PublicSearchTimerPreviewRequest
{
    std::string backendId;
    std::string searchTimerId;
    std::size_t limit = 30U;
    // Only SearchTimerPreviewService-supported EPG match fields. Never a
    // simulated backend CREATE, duplicate/blacklist or recording planner.
    bool draft = false;
    std::string query;
    int mode = 0;
    int tolerance = 0;
    bool compareTitle = true;
    bool compareSubtitle = false;
    bool compareSummary = false;
};

struct PublicSearchTimerPreviewResult
{
    PublicSearchTimerPreviewStatus status =
        PublicSearchTimerPreviewStatus::unavailable;
    std::string backendId;
    std::string searchTimerId;
    int totalCount = 0;
    std::vector<PublicSearchTimerPreviewEvent> items;
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
    bool edited = false;
    // Read-only descriptive facts; no artwork/provider URL or native path.
    std::string description;
    long long sizeMb = 0;
};

struct PublicRecordingFolderItem
{
    std::string folderId;
    std::string name;
    int recordingCount = 0;
    // The Web Home hierarchy marks title folders containing one recording
    // as recording leaves. Preserve that distinction in Public-v1.
    bool singleRecordingLeaf = false;
    PublicRecordingCollectionItem singleRecording;
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

// M1: one public rec_ detail under an exact recordings.view backend grant.
// The daemon owns the cache and public-ID resolution; HTTP never receives
// a native Recording path or any provider artwork cache reference.
struct PublicRecordingDetailRequest
{
    std::string backendId;
    std::string recordingId;
};

enum class PublicRecordingDetailStatus { ok, invalid, notFound, unavailable };

struct PublicRecordingDetailItem
{
    PublicRecordingCollectionItem recording;
    std::string metadataState;
    std::string metadataSource;
    std::string contentKind;
    std::string epgTitle;
    std::string subtitle;
    std::string epgDescription;
    std::string providerTitle;
    std::string overview;
    std::string originalTitle;
    std::string tagline;
    std::string seriesTitle;
    std::string episodeTitle;
    std::string genreText;
    std::string manualGenreId;
    std::string releaseDate;
    int seasonNumber = 0;
    int episodeNumber = 0;
    int providerRuntimeMinutes = 0;
    std::string manualAssignmentId;
    std::string metadataEntityId;
    int manualRevision = 0;
};

struct PublicRecordingDetailResult
{
    PublicRecordingDetailStatus status = PublicRecordingDetailStatus::unavailable;
    PublicRecordingDetailItem item;
};

// M2: Read-only Recording artwork, projected from the existing cache.
// Public consumers never see native paths, provider IDs or credentials.
struct PublicRecordingArtworkRequest
{
    std::string backendId;
    std::string recordingId;
    std::string artworkId; // empty for list, opaque asset ID for image bytes
};
enum class PublicRecordingArtworkStatus { ok, invalid, notFound, unavailable };
struct PublicRecordingArtworkItem
{
    std::string artworkId;
    std::string kind;
    int width = 0;
    int height = 0;
};
struct PublicRecordingArtworkResult
{
    PublicRecordingArtworkStatus status = PublicRecordingArtworkStatus::unavailable;
    std::vector<PublicRecordingArtworkItem> items;
    std::string contentType;
    std::string content;
};

enum class PublicSearchStatus { ok, unavailable };
struct PublicSearchRequest {
    std::string backendId, query;
    bool epg = false;
    std::size_t limit = 30U, offset = 0U;
};
struct PublicSearchItem {
    std::string backendId, recordingId, channelId, eventId;
    std::string title, subtitle, startTime, endTime;
    bool edited = false;
};
struct PublicSearchResult {
    PublicSearchStatus status = PublicSearchStatus::unavailable;
    std::vector<PublicSearchItem> items;
    std::size_t totalCount = 0U;
    bool hasMore = false;
};

enum class PublicEpgGenreStatus { ok, unavailable, invalid };
struct PublicEpgGenreRequest {
    std::string backendId, contentClass, genreId, locale = "de";
    std::size_t limit = 30U, offset = 0U;
    bool events = false;
};
struct PublicEpgGenreItem {
    std::string genreId, contentClass, label, labelDe, labelEn;
    std::size_t count = 0U;
};
struct PublicEpgGenreEvent {
    std::string backendId, channelId, eventId, title, subtitle;
    std::string startTime, endTime;
};
struct PublicEpgGenreResult {
    PublicEpgGenreStatus status = PublicEpgGenreStatus::unavailable;
    std::vector<PublicEpgGenreItem> genres;
    std::vector<PublicEpgGenreEvent> events;
    std::size_t totalCount = 0U;
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
    bool edited = false;
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
    notFound,
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
    // Additive list order; false preserves legacy backendId/channelId pages.
    bool nativeChannelOrder = false;
    int afterChannelNumber = 0;
    // Optional single-backend numeric VDR neighbor read; no ch1_ cursor.
    std::string relativeToChannelId;
    std::string direction;
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
    // Appended to preserve earlier aggregate field positions.
    // Never fabricate an ID from programme title or timestamps.
    std::string eventId;
};

struct PublicEpgNowNextResult
{
    PublicEpgNowNextStatus status = PublicEpgNowNextStatus::unavailable;
    std::vector<PublicEpgNowNextItem> events;
};

// A bounded read from the existing EPG cache for one Device-authorized
// channel. Explicit time range, no private Browser route or source URL.
struct PublicEpgWindowRequest
{
    std::string backendId;
    std::string channelId;
    std::string fromTime;
    std::string untilTime;
    std::size_t limit = 192U;
    // Private cache keyset, decoded from a scope- and window-bound cursor.
    std::string afterStartTime;
    std::string afterCacheEventId;
};

struct PublicEpgWindowResult
{
    PublicEpgNowNextStatus status = PublicEpgNowNextStatus::unavailable;
    std::vector<PublicEpgNowNextItem> events;
    bool hasMore = false;
    // These values identify the last cache row, not a native source EventID.
    std::string nextStartTime;
    std::string nextCacheEventId;
};

// Read-only, exact source-provenanced EPG airing; no Timer CREATE authority.
struct PublicEpgEventDetailRequest
{
    std::string backendId, channelId, eventId;
};
struct PublicEpgEventDetailResult
{
    PublicEpgNowNextStatus status = PublicEpgNowNextStatus::unavailable;
    std::string backendId, channelId, eventId, title, subtitle;
    std::string description, startTime, endTime, resourceRevision;
    int durationSeconds = 0;
};

// Snapshot-backed, read-only public list of real VDR timers. These are
// native timers, never Suite TimerAssignments or search timer definitions.
enum class PublicNativeTimersStatus { ok, invalid, unavailable };
struct PublicNativeTimersRequest
{
    std::string backendId;
    std::size_t offset = 0U;
    std::size_t limit = 30U;
};
struct PublicNativeTimerItem
{
    std::string id;
    std::string channelId;
    std::string channelName;
    std::string title;
    std::string day;
    std::string weekdays;
    std::string startTime;
    std::string endTime;
    bool enabled = false;
    bool recording = false;
    bool pending = false;
};
struct PublicNativeTimersResult
{
    PublicNativeTimersStatus status = PublicNativeTimersStatus::unavailable;
    std::vector<PublicNativeTimerItem> timers;
    bool hasMore = false;
};

enum class PublicNativeTimerDetailStatus { ok, notFound, unavailable };
struct PublicNativeTimerDetailRequest
{
    std::string backendId;
    std::string timerId;
};
struct PublicNativeTimerDetailResult
{
    PublicNativeTimerDetailStatus status = PublicNativeTimerDetailStatus::unavailable;
    PublicNativeTimerItem timer;
    std::string backendId;
    std::string ownership = "unbound";
    std::string bindingState = "binding_required";
    std::string resourceRevision; // Empty for unbound, never a guessed ETag.
};

enum class PublicNativeTimerDeleteStatus {
    accepted, replayed, invalid, notFound, revisionConflict,
    adoptionRequired, stateConflict, idempotencyConflict,
    backendUnavailable, serviceUnavailable
};
struct PublicNativeTimerDeleteRequest {
    std::string actorRef;
    std::string backendId;
    std::string timerId;
    std::string expectedResourceRevision;
    std::string idempotencyKey;
};
struct PublicNativeTimerDeleteResult {
    PublicNativeTimerDeleteStatus status = PublicNativeTimerDeleteStatus::serviceUnavailable;
    PublicOperationResource operation;
};

enum class PublicNativeTimerModifyStatus {
    accepted, replayed, invalid, notFound, revisionConflict,
    adoptionRequired, stateConflict, idempotencyConflict,
    backendUnavailable, serviceUnavailable
};
enum class PublicNativeTimerChangeKind { update, toggle };
struct PublicNativeTimerChangeFields
{
    std::string channelId;
    std::string title;
    std::string directory;
    std::string day;
    std::string weekdays = "-------";
    std::string startTime;
    std::string endTime;
    std::int32_t priority = 50;
    std::int32_t lifetime = 99;
    bool enabled = true;
    bool vps = false;
};
struct PublicNativeTimerModifyRequest {
    std::string actorRef;
    std::string backendId;
    std::string timerId;
    std::string expectedResourceRevision;
    std::string idempotencyKey;
    PublicNativeTimerChangeKind kind = PublicNativeTimerChangeKind::update;
    PublicNativeTimerChangeFields specification;
};
struct PublicNativeTimerModifyResult {
    PublicNativeTimerModifyStatus status = PublicNativeTimerModifyStatus::serviceUnavailable;
    PublicOperationResource operation;
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

// Administrator-only catalogue, populated from the canonical security_devices table.
struct PublicDeviceCollectionRequest
{
    std::string afterDeviceId;
    std::size_t limit = 50U;
};

struct PublicDeviceCollectionItem
{
    std::string deviceId;
    std::string actorId;
    std::string displayName;
    bool active = false;
    bool revoked = false;
};

struct PublicDeviceCollectionResult
{
    bool available = false;
    std::vector<PublicDeviceCollectionItem> items;
    bool hasMore = false;
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

// A new EPG intent is not an existing TimerAssignment and is never a
// caller-supplied native Timer specification.
struct PublicEpgTimerIntentAdmissionRequest
{
    std::string actorRef;
    vdrsuite::public_epg_timer::Selection selection;
};
enum class PublicEpgTimerIntentObservationStatus { ok, notFound, unavailable };
struct PublicEpgTimerIntentObservationResult
{
    PublicEpgTimerIntentObservationStatus status =
        PublicEpgTimerIntentObservationStatus::unavailable;
    vdrsuite::public_epg_timer::AuthoritativeObservation observation;
    bool backendAllowsMutation = false;
};
enum class PublicEpgTimerIntentAdmissionStatus
{
    accepted, replayed, invalid, notFound, revisionConflict,
    generationConflict, stateConflict, idempotencyConflict,
    backendUnavailable, serviceUnavailable
};
struct PublicEpgTimerIntentAdmissionResult
{
    PublicEpgTimerIntentAdmissionStatus status =
        PublicEpgTimerIntentAdmissionStatus::serviceUnavailable;
    PublicOperationResource operation;
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

enum class PublicMediaImportAdmissionStatus
{
    accepted, replayed, invalid, forbidden, idempotencyConflict,
    conflict, unavailable
};

struct PublicMediaImportAdmissionRequest
{
    std::string actorRef;
    std::string backendId;
    std::string sourceId;
    std::string idempotencyKey;
    bool approveVideoReencode = false; // immutable, explicitly selected
    std::string videoDecision; // optional "desktop", "skip", "duplicate" or "release"
};

struct PublicMediaImportAdmissionResult
{
    PublicMediaImportAdmissionStatus status =
        PublicMediaImportAdmissionStatus::unavailable;
    PublicOperationResource operation;
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

    using SearchTimerCollectionLookup =
        std::function<PublicSearchTimerCollectionResult(
            const PublicSearchTimerCollectionRequest& request)>;

    using SearchTimerCreate =
        std::function<PublicSearchTimerCreateResult(
            const PublicSearchTimerCreateRequest& request)>;

    using SearchTimerNativeVersionLookup =
        std::function<PublicSearchTimerNativeVersionResult(
            const PublicSearchTimerNativeVersionRequest&)>;
    using SearchTimerNativeMutation =
        std::function<PublicSearchTimerNativeMutationResult(
            const PublicSearchTimerNativeMutationRequest&)>;

    using SearchTimerPreviewLookup =
        std::function<PublicSearchTimerPreviewResult(
            const PublicSearchTimerPreviewRequest& request)>;

    using RecordingCollectionLookup =
        std::function<PublicRecordingCollectionResult(
            const PublicRecordingCollectionRequest& request)>;

    using RecordingDetailLookup =
        std::function<PublicRecordingDetailResult(
            const PublicRecordingDetailRequest&)>;

    using RecordingArtworkLookup =
        std::function<PublicRecordingArtworkResult(
            const PublicRecordingArtworkRequest&)>;

    using SearchLookup =
        std::function<PublicSearchResult(const PublicSearchRequest&)>;

    using EpgGenreLookup =
        std::function<PublicEpgGenreResult(const PublicEpgGenreRequest&)>;

    using GenreCollectionLookup =
        std::function<PublicGenreCollectionResult(
            const PublicGenreCollectionRequest& request)>;

    using ChannelCollectionLookup =
        std::function<PublicChannelCollectionResult(
            const PublicChannelCollectionRequest& request)>;

    using EpgNowNextLookup =
        std::function<PublicEpgNowNextResult(
            const PublicEpgNowNextRequest& request)>;

    using EpgWindowLookup =
        std::function<PublicEpgWindowResult(
            const PublicEpgWindowRequest& request)>;
    using EpgEventDetailLookup = std::function<PublicEpgEventDetailResult(
        const PublicEpgEventDetailRequest&)>;
    using NativeTimersLookup =
        std::function<PublicNativeTimersResult(
            const PublicNativeTimersRequest& request)>;

    using NativeTimerDetailLookup = std::function<PublicNativeTimerDetailResult(
        const PublicNativeTimerDetailRequest&)>;
    using NativeTimerDeleteAdmission = std::function<PublicNativeTimerDeleteResult(
        const PublicNativeTimerDeleteRequest&)>;
    using NativeTimerModifyAdmission = std::function<PublicNativeTimerModifyResult(
        const PublicNativeTimerModifyRequest&)>;
    // A7.2: the Core actor/backend authority is the only source of truth.
    enum class MediaImportSourceStatus { ok, forbidden, unavailable };
    struct MediaImportSourceResult {
        MediaImportSourceStatus status = MediaImportSourceStatus::unavailable;
        std::vector<std::string> ids;
        std::vector<std::string> labels; // optional authorized display names
        std::vector<std::string> folders; // optional source-root-relative folders
        std::vector<std::string> statuses; // advisory recording_present/unverified
        bool partial = true; // fail closed for legacy/unknown observations
    };
    using MediaImportSourceLookup = std::function<MediaImportSourceResult(
        const std::string& actorId, const std::string& backendId)>;

    enum class MediaImportPreviewStatus { pending, ready, forbidden, unavailable };
    struct MediaImportPreviewResult {
        MediaImportPreviewStatus status = MediaImportPreviewStatus::unavailable;
        std::string sourceId;
        std::string inspectionStatus;
        std::string title;
        std::string targetFolder;
        std::string videoCodec;
        std::uint64_t sourceBytes = 0;
        std::uint64_t availableBytes = 0;
        std::uint64_t requiredBytes = 0;
        bool videoReencodeRequired = false;
        std::string failureCode;
        std::string nfoOutline, nfoPlot, nfoYear, nfoGenre;
        std::uint64_t durationSeconds = 0;
        std::string posterDataUri, fanartDataUri;
    };
    using MediaImportPreviewLookup = std::function<MediaImportPreviewResult(
        const std::string& actorId, const std::string& backendId,
        const std::string& sourceId)>;

    using MediaImportCapabilityLookup =
        std::function<bool(const std::string& actorId,
                           const std::string& backendId)>;

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

    using DeviceCollectionLookup =
        std::function<PublicDeviceCollectionResult(
            const PublicDeviceCollectionRequest& request)>;

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

    using EpgTimerIntentObservationLookup =
        std::function<PublicEpgTimerIntentObservationResult(
            const PublicEpgTimerIntentAdmissionRequest&)>;
    using EpgTimerIntentReplayLookup =
        std::function<PublicEpgTimerIntentAdmissionResult(
            const PublicEpgTimerIntentAdmissionRequest&)>;
    using EpgTimerIntentSubmit =
        std::function<PublicEpgTimerIntentAdmissionResult(
            const PublicEpgTimerIntentAdmissionRequest&,
            const vdrsuite::public_epg_timer::AuthoritativeObservation&)>;

    using MediaImportAdmission =
        std::function<PublicMediaImportAdmissionResult(
            const PublicMediaImportAdmissionRequest& request)>;

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

    void registerSearchTimerCollectionLookup(
        SearchTimerCollectionLookup lookup);
    void resetSearchTimerCollectionLookup();
    bool searchTimerCollectionLookupConfigured() const;
    PublicSearchTimerCollectionResult lookupSearchTimerCollection(
        const PublicSearchTimerCollectionRequest& request) const;

    void registerSearchTimerCreate(SearchTimerCreate handler);
    void resetSearchTimerCreate();

    void registerSearchTimerNativeVersionLookup(
        SearchTimerNativeVersionLookup lookup);
    void resetSearchTimerNativeVersionLookup();
    void registerSearchTimerNativeMutation(SearchTimerNativeMutation handler);
    void resetSearchTimerNativeMutation();

    void registerSearchTimerPreviewLookup(SearchTimerPreviewLookup lookup);
    void resetSearchTimerPreviewLookup();
    PublicSearchTimerPreviewResult lookupSearchTimerPreview(
        const PublicSearchTimerPreviewRequest& request) const;

    void registerRecordingCollectionLookup(RecordingCollectionLookup lookup);
    void resetRecordingCollectionLookup();
    bool recordingCollectionLookupConfigured() const;
    PublicRecordingCollectionResult lookupRecordingCollection(
        const PublicRecordingCollectionRequest& request) const;

    void registerRecordingDetailLookup(RecordingDetailLookup lookup);
    void resetRecordingDetailLookup();
    PublicRecordingDetailResult lookupRecordingDetail(
        const PublicRecordingDetailRequest& request) const;

    void registerRecordingArtworkLookup(RecordingArtworkLookup lookup);
    void resetRecordingArtworkLookup();
    PublicRecordingArtworkResult lookupRecordingArtwork(
        const PublicRecordingArtworkRequest& request) const;

    void registerSearchLookup(SearchLookup lookup);
    void resetSearchLookup();
    bool searchLookupConfigured() const;
    PublicSearchResult lookupSearch(const PublicSearchRequest& request) const;

    void registerEpgGenreLookup(EpgGenreLookup lookup);
    void resetEpgGenreLookup();
    bool epgGenreLookupConfigured() const;
    PublicEpgGenreResult lookupEpgGenre(const PublicEpgGenreRequest&) const;

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

    void registerEpgWindowLookup(EpgWindowLookup lookup);
    void resetEpgWindowLookup();
    bool epgWindowLookupConfigured() const;
    PublicEpgWindowResult lookupEpgWindow(
        const PublicEpgWindowRequest& request) const;
    void registerEpgEventDetailLookup(EpgEventDetailLookup lookup);
    void resetEpgEventDetailLookup();
    bool epgEventDetailLookupConfigured() const;
    PublicEpgEventDetailResult lookupEpgEventDetail(
        const PublicEpgEventDetailRequest& request) const;
    void registerNativeTimersLookup(NativeTimersLookup lookup);
    void resetNativeTimersLookup();
    bool nativeTimersLookupConfigured() const;
    PublicNativeTimersResult lookupNativeTimers(
        const PublicNativeTimersRequest& request) const;

    void registerNativeTimerDetailLookup(NativeTimerDetailLookup callback);
    void resetNativeTimerDetailLookup();
    PublicNativeTimerDetailResult lookupNativeTimerDetail(
        const PublicNativeTimerDetailRequest& request) const;

    void registerNativeTimerModifyAdmission(NativeTimerModifyAdmission admit);
    void resetNativeTimerModifyAdmission();
    void registerNativeTimerDeleteAdmission(NativeTimerDeleteAdmission admit);
    void resetNativeTimerDeleteAdmission();
    bool tryHandleDelete(
        const std::string& requestTarget,
        const std::string& actorRef,
        const std::string& requestId,
        const std::string& correlationId,
        const std::string& ifMatch,
        const std::string& idempotencyKey,
        const std::string& authorizedBackendId,
        ApiResponse& response) const;

    void registerMediaImportSourceLookup(MediaImportSourceLookup lookup);
    void resetMediaImportSourceLookup();
    MediaImportSourceResult lookupMediaImportSources(
        const std::string& actorId, const std::string& backendId) const;

    void registerMediaImportPreviewLookup(MediaImportPreviewLookup lookup);
    void resetMediaImportPreviewLookup();
    MediaImportPreviewResult lookupMediaImportPreview(
        const std::string& actorId, const std::string& backendId,
        const std::string& sourceId) const;

    void registerMediaImportCapabilityLookup(MediaImportCapabilityLookup lookup);
    void resetMediaImportCapabilityLookup();
    bool lookupMediaImportCapability(const std::string& actorId,
                                     const std::string& backendId) const;

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

    void registerDeviceCollectionLookup(DeviceCollectionLookup lookup);
    void resetDeviceCollectionLookup();
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

    void registerEpgTimerIntentObservationLookup(EpgTimerIntentObservationLookup callback);
    void resetEpgTimerIntentObservationLookup();
    void registerEpgTimerIntentReplayLookup(EpgTimerIntentReplayLookup callback);
    void resetEpgTimerIntentReplayLookup();
    void registerEpgTimerIntentSubmit(EpgTimerIntentSubmit callback);
    void resetEpgTimerIntentSubmit();
    bool epgTimerIntentAdmissionConfigured() const;

    void registerMediaImportAdmission(MediaImportAdmission admission);
    void resetMediaImportAdmission();

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

    // ADR-0068: mutation route ownership only. Auth, callback admission,
    // revisions, idempotency and response construction remain unchanged.
    bool tryHandleNativeTimerMutationPost(
        const std::string& requestTarget,
        const std::string& requestId,
        const std::string& correlationId,
        ApiResponse& response,
        const std::string& body,
        const std::string& actorRef,
        const std::string& ifMatch,
        const std::string& idempotencyKey,
        const std::string& contentType,
        const std::string& authorizedBackendId) const;

    bool tryHandleDevicePairingPost(
        const std::string& requestTarget,
        const std::string& requestId,
        const std::string& correlationId,
        ApiResponse& response,
        const std::string& body,
        const std::string& actorRef,
        const std::string& ifMatch,
        const std::string& idempotencyKey,
        const std::string& contentType,
        const std::string& pairingToken) const;

    bool tryHandleAccountCredentialSessionPost(
        const std::string& requestTarget,
        const std::string& requestId,
        const std::string& correlationId,
        ApiResponse& response,
        const std::string& body,
        const std::string& actorRef,
        const std::string& ifMatch,
        const std::string& contentType) const;

    bool tryHandleDeviceAdministrationPost(
        const std::string& requestTarget,
        const std::string& requestId,
        const std::string& correlationId,
        ApiResponse& response,
        const std::string& body,
        const std::string& actorRef,
        const std::string& ifMatch,
        const std::string& idempotencyKey,
        const std::string& contentType) const;

    bool tryHandleAccountAdministrationPost(
        const std::string& requestTarget,
        const std::string& requestId,
        const std::string& correlationId,
        ApiResponse& response,
        const std::string& body,
        const std::string& actorRef,
        const std::string& ifMatch,
        const std::string& idempotencyKey,
        const std::string& contentType) const;

    bool tryHandleTimerWorkflowPost(
        const std::string& requestTarget,
        const std::string& requestId,
        const std::string& correlationId,
        ApiResponse& response,
        const std::string& body,
        const std::string& actorRef,
        const std::string& ifMatch,
        const std::string& idempotencyKey,
        const std::string& contentType,
        const std::string& authorizedBackendId) const;

    // Human Account / Device administration GET on the established
    // identity runtime, with unchanged revisions and Problem Details.
    bool tryHandleIdentityAdministrationReadGet(
        const std::string& requestTarget,
        const std::string& actorRef,
        const std::string& requestId,
        const std::string& correlationId,
        const std::string& ifNoneMatch,
        ApiResponse& response) const;

    // Pairing collection and item reads preserve human/pairing-token gates.
    bool tryHandleDevicePairingReadGet(
        const std::string& requestTarget,
        const std::string& actorRef,
        const std::string& requestId,
        const std::string& correlationId,
        const std::string& pairingToken,
        ApiResponse& response) const;

    // Recording, EPG/Recording search and Genre read responses use the
    // established runtime lookup callbacks.
    bool tryHandleBrowseReadGet(
        const std::string& requestTarget,
        const std::string& actorRef,
        const std::string& requestId,
        const std::string& correlationId,
        const std::vector<std::string>& authorizedBackendIds,
        ApiResponse& response) const;

    // SearchTimer draft/resource previews, collection and TimerAssignment
    // collection/item GET share the existing runtime callback authorities.
    bool tryHandleSearchTimerAssignmentReadGet(
        const std::string& requestTarget,
        const std::string& actorRef,
        const std::string& requestId,
        const std::string& correlationId,
        const std::string& ifNoneMatch,
        const std::string& authorizedBackendId,
        ApiResponse& response) const;

    // Original NativeTimer collection/item GET contract; no new timer owner.
    bool tryHandleNativeTimerReadGet(
        const std::string& requestTarget,
        const std::string& actorRef,
        const std::string& requestId,
        const std::string& correlationId,
        const std::string& ifNoneMatch,
        const std::vector<std::string>& authorizedBackendIds,
        ApiResponse& response) const;

    // EPG Event, Window, Now/Next read-only orchestration on this runtime.
    bool tryHandleEpgReadGet(
        const std::string& requestTarget,
        const std::string& actorRef,
        const std::string& requestId,
        const std::string& correlationId,
        const std::string& ifNoneMatch,
        const std::vector<std::string>& authorizedBackendIds,
        ApiResponse& response) const;

    // The Channel GET path retains its existing parse, scope and cursor logic.
    bool tryHandleChannelCollectionGet(
        const std::string& requestTarget,
        const std::string& actorRef,
        const std::string& requestId,
        const std::string& correlationId,
        const std::vector<std::string>& authorizedBackendIds,
        ApiResponse& response) const;

    // Backend read admission and paging remain on this runtime instance.
    bool tryHandleBackendCollectionGet(
        const std::string& requestTarget,
        const std::string& actorRef,
        const std::string& requestId,
        const std::string& correlationId,
        const std::vector<std::string>& authorizedBackendIds,
        ApiResponse& response) const;

    PublicOperationLookupResult lookupOperation(
        const std::string& operationId,
        const std::string& actorRef) const;

    mutable std::mutex operationLookupMutex_;
    OperationLookup operationLookup_;

    mutable std::mutex timerAssignmentLookupMutex_;
    TimerAssignmentLookup timerAssignmentLookup_;

    mutable std::mutex timerAssignmentCollectionLookupMutex_;
    TimerAssignmentCollectionLookup timerAssignmentCollectionLookup_;

    mutable std::mutex searchTimerCollectionLookupMutex_;
    SearchTimerCollectionLookup searchTimerCollectionLookup_;

    mutable std::mutex searchTimerCreateMutex_;
    SearchTimerCreate searchTimerCreate_;

    mutable std::mutex searchTimerNativeVersionMutex_;
    SearchTimerNativeVersionLookup searchTimerNativeVersion_;
    mutable std::mutex searchTimerNativeMutationMutex_;
    SearchTimerNativeMutation searchTimerNativeMutation_;

    mutable std::mutex searchTimerPreviewLookupMutex_;
    SearchTimerPreviewLookup searchTimerPreviewLookup_;

    mutable std::mutex recordingCollectionLookupMutex_;
    RecordingCollectionLookup recordingCollectionLookup_;

    mutable std::mutex recordingDetailLookupMutex_;
    RecordingDetailLookup recordingDetailLookup_;

    mutable std::mutex recordingArtworkLookupMutex_;
    RecordingArtworkLookup recordingArtworkLookup_;

    mutable std::mutex searchLookupMutex_;
    SearchLookup searchLookup_;

    mutable std::mutex epgGenreLookupMutex_;
    EpgGenreLookup epgGenreLookup_;

    mutable std::mutex genreCollectionLookupMutex_;
    GenreCollectionLookup genreCollectionLookup_;

    mutable std::mutex channelCollectionLookupMutex_;
    ChannelCollectionLookup channelCollectionLookup_;

    mutable std::mutex epgNowNextLookupMutex_;
    EpgNowNextLookup epgNowNextLookup_;

    mutable std::mutex epgWindowLookupMutex_;
    EpgWindowLookup epgWindowLookup_;
    mutable std::mutex epgEventDetailMutex_;
    EpgEventDetailLookup epgEventDetailLookup_;
    mutable std::mutex nativeTimersLookupMutex_;
    NativeTimersLookup nativeTimersLookup_;

    mutable std::mutex nativeTimerDetailMutex_;
    NativeTimerDetailLookup nativeTimerDetailLookup_;
    mutable std::mutex nativeTimerDeleteMutex_;
    NativeTimerDeleteAdmission nativeTimerDeleteAdmission_;
    mutable std::mutex nativeTimerModifyMutex_;
    NativeTimerModifyAdmission nativeTimerModifyAdmission_;
    mutable std::mutex mediaImportSourceLookupMutex_;
    MediaImportSourceLookup mediaImportSourceLookup_;
    mutable std::mutex mediaImportPreviewLookupMutex_;
    MediaImportPreviewLookup mediaImportPreviewLookup_;

    mutable std::mutex mediaImportCapabilityLookupMutex_;
    MediaImportCapabilityLookup mediaImportCapabilityLookup_;

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

    mutable std::mutex deviceCollectionLookupMutex_;
    DeviceCollectionLookup deviceCollectionLookup_;

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

    mutable std::mutex epgTimerIntentMutex_;
    EpgTimerIntentObservationLookup epgTimerIntentObservationLookup_;
    EpgTimerIntentReplayLookup epgTimerIntentReplayLookup_;
    EpgTimerIntentSubmit epgTimerIntentSubmit_;

    mutable std::mutex mediaImportAdmissionMutex_;
    MediaImportAdmission mediaImportAdmission_;

    mutable std::mutex timerCreateAdmissionMutex_;
    TimerCreateAdmission timerCreateAdmission_;
};
