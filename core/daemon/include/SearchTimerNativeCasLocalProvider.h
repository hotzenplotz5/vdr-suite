#pragma once

#include "SearchTimerNativeCasContract.h"
#include "SuiteBridgeLocalControlTransport.h"

#include <string>

struct SearchTimerNativeCasLocalObservation
{
    bool available = false;
    bool found = false;
    bool durable = false;
    std::string generation;
    std::string revision;
    std::string settings;
};

// Concrete, strictly local backend adapter. Remote backends must use their own
// authorized Agent transport; never route a different backend to local VDR.
// The caller still must perform durable admission and exclusive dispatch claim.
class SearchTimerNativeCasLocalProvider final : public ISearchTimerNativeCasProvider {
public:
    SearchTimerNativeCasLocalProvider(
        std::string localBackendId,
        vdrsuite::agent::SuiteBridgeLocalControlTransport& transport);

    bool supportsAtomicGenerationCasV1() const override;
    SearchTimerNativeCasLocalObservation observeRule(
        const std::string& backendId, int nativeId);

    // Backend-only safe update builder: keeps every untouched native field,
    // refuses active rules and never accepts a client-provided native ID.
    // Colon/control bytes require the dedicated epgsearch native encoder and
    // are deliberately rejected here instead of guessing an escape format.
    static bool buildInactiveQueryUpdate(
        const SearchTimerNativeCasLocalObservation& original,
        int nativeId, const std::string& query, std::string& candidate);
    SearchTimerNativeCasReply compareAndWrite(
        const SearchTimerNativeCasRequest& request) override;
private:
    std::string localBackendId_;
    vdrsuite::agent::SuiteBridgeLocalControlTransport& transport_;
};
