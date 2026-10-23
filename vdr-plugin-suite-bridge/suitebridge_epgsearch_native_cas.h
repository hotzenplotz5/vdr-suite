#ifndef VDR_SUITE_BRIDGE_EPGSEARCH_NATIVE_CAS_H
#define VDR_SUITE_BRIDGE_EPGSEARCH_NATIVE_CAS_H

#include <string>

// ABI mirror of the *pinned*, versioned epgsearch services in patches/epgsearch.
// Service() is a void* boundary across plugins; field order, enum values and
// standard-library ABI MUST match the patched services.h exactly. CI checks
// the mirrored declarations against the native patch series before shipping.
// No legacy SVDRP/RESTfulAPI writer is an acceptable fallback.
struct Epgsearch_searchtimer_observe_v1_0 {
    int nativeId;
    bool found;
    bool generationPersisted;
    std::string generation;
    std::string settings;
};

struct Epgsearch_searchtimer_observe_v1_1 : Epgsearch_searchtimer_observe_v1_0 {
    bool revisionPersisted;
    std::string revision;
};

enum eEpgsearchNativeToggleCasStatus {
    entcsUnavailable = 0,
    entcsApplied,
    entcsNotFound,
    entcsStaleGeneration,
    entcsStaleRevision,
    entcsUnpersistedChange,
    entcsOutcomeUnknown
};

struct Epgsearch_searchtimer_toggle_cas_v1_0 {
    int nativeId;
    std::string expectedGeneration;
    std::string expectedRevision;
    bool activate;
    eEpgsearchNativeToggleCasStatus status;
    bool atomicCompareAndWrite;
    bool noWriteAttested;
    std::string confirmedGeneration;
    std::string resultingRevision;
};

enum eEpgsearchNativeUpdateCasStatus {
    enucsUnavailable = 0,
    enucsApplied,
    enucsInvalid,
    enucsNotFound,
    enucsStaleGeneration,
    enucsStaleRevision,
    enucsNotInactive,
    enucsUnpersistedChange,
    enucsOutcomeUnknown
};

struct Epgsearch_searchtimer_update_cas_v1_0 {
    int nativeId;
    std::string expectedGeneration;
    std::string expectedRevision;
    std::string settings;
    eEpgsearchNativeUpdateCasStatus status;
    bool atomicCompareAndWrite;
    bool noWriteAttested;
    std::string confirmedGeneration;
    std::string resultingRevision;
};

enum eEpgsearchNativeDeleteCasStatus {
    endcsUnavailable = 0,
    endcsApplied,
    endcsNotFound,
    endcsStaleGeneration,
    endcsStaleRevision,
    endcsUnpersistedChange,
    endcsOutcomeUnknown
};

struct Epgsearch_searchtimer_delete_cas_v1_0 {
    int nativeId;
    std::string expectedGeneration;
    std::string expectedRevision;
    eEpgsearchNativeDeleteCasStatus status;
    bool atomicCompareAndWrite;
    bool noWriteAttested;
    std::string confirmedDeletedGeneration;
};

enum class SuiteBridgeSearchCasAction { Update, Activate, Deactivate, Delete };

enum class SuiteBridgeSearchCasStatus {
    Unavailable, Invalid, NotFound, StaleGeneration, StaleRevision,
    NotInactive, UnpersistedChange, AppliedVerified, OutcomeUnknown
};

struct SuiteBridgeSearchCasRequest {
    SuiteBridgeSearchCasAction action = SuiteBridgeSearchCasAction::Update;
    int nativeId = -1;
    std::string generation;
    std::string revision;
    std::string settings;
};

struct SuiteBridgeSearchCasObservation {
    bool available = false;
    bool found = false;
    bool durable = false;
    std::string generation;
    std::string revision;
    std::string settings;
};

struct SuiteBridgeSearchCasResult {
    SuiteBridgeSearchCasStatus status = SuiteBridgeSearchCasStatus::Unavailable;
    bool atomicCompareAndWrite = false;
    bool noWriteAttested = false;
    std::string generation;
    std::string revision;
};

class SuiteBridgeEpgsearchNativeCas final {
public:
    // Same-process native epgsearch provider; no SVDRP write endpoint.
    // The caller must FIRST prove actor/backend permissions and an exclusive
    // persisted one-shot operation claim; this class is not that authority.
    bool Available() const;
    SuiteBridgeSearchCasObservation Observe(int nativeId) const;
    SuiteBridgeSearchCasResult Execute(
        const SuiteBridgeSearchCasRequest &request) const;
};

#endif
