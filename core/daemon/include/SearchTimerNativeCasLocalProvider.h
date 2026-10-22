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
    SearchTimerNativeCasReply compareAndWrite(
        const SearchTimerNativeCasRequest& request) override;
private:
    std::string localBackendId_;
    vdrsuite::agent::SuiteBridgeLocalControlTransport& transport_;
};
