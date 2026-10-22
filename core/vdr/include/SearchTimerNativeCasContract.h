#pragma once

#include <string>

// A versioned protocol contract for a FUTURE epgsearch-native conditional
// writer. The current numeric-ID-only RESTfulAPI/SVDRP/Service writers MUST
// NOT implement this interface. Nothing here enables a Public-v1 write route.
// Each caller must first claim a durable, actor-scoped mutation operation.
enum class SearchTimerNativeCasAction
{
    update,
    activate,
    deactivate,
    remove,
};

enum class SearchTimerNativeCasStatus
{
    applied,
    staleGeneration,
    staleRevision,
    notFound,
    unavailable,
    unsupported,
    outcomeUnknown,
    invalid,
    denied,
};

struct SearchTimerNativeCasRequest
{
    SearchTimerNativeCasAction action = SearchTimerNativeCasAction::update;
    std::string operationId;
    std::string backendId;
    std::string publicSearchTimerId;
    std::string nativeId;
    // Must be minted/stored by the native epgsearch plugin, not calculated
    // from the numeric nativeId or the user-editable rule settings.
    std::string expectedGeneration;
    std::string expectedRevision;
    std::string requestFingerprint;
    // A provider-independent opaque payload; future integration must verify
    // exact durable payload binding before filling this request.
    std::string changePayload;
    bool durableOperationClaimed = false;
    bool backendWriteAuthorized = false;
    bool operatorConfirmed = false;
};

struct SearchTimerNativeCasReply
{
    SearchTimerNativeCasStatus status = SearchTimerNativeCasStatus::unavailable;
    // On success the provider must attest the SAME generation, and a new
    // revision (or exact native deletion for remove) under its own rule lock.
    std::string confirmedGeneration;
    std::string resultingRevision;
    bool atomicCompareAndWrite = false;
    // Negative native CAS results require an explicit zero-write attestation
    // under the SAME epgsearch-owned rule lock as the comparison.
    bool noWriteAttested = false;
};

class ISearchTimerNativeCasProvider
{
public:
    virtual ~ISearchTimerNativeCasProvider() = default;
    // Explicit provider feature negotiation. No legacy fallback.
    virtual bool supportsAtomicGenerationCasV1() const = 0;
    virtual SearchTimerNativeCasReply compareAndWrite(
        const SearchTimerNativeCasRequest& request) = 0;
};

class SearchTimerNativeCasContract final
{
public:
    // Pre-admission shape check. The operation ID/fingerprint are verified
    // separately after immutable operation reservation.
    static bool validNativeEvidence(
        const SearchTimerNativeCasRequest& request);

    // One attempted native write *after* a durable operation CAS has been
    // claimed by the caller. This does not persist idempotency itself.
    // Missing native capability, invalid fence or authorization => 0 calls.
    static SearchTimerNativeCasStatus invokeOnce(
        const SearchTimerNativeCasRequest& request,
        ISearchTimerNativeCasProvider& provider);
};
