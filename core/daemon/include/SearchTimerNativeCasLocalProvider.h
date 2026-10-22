#pragma once

#include "SearchTimerNativeCasContract.h"
#include "SuiteBridgeLocalControlTransport.h"

#include <string>

// Concrete, strictly local backend adapter. Remote backends must use their own
// authorized Agent transport; never route a different backend to local VDR.
// The caller still must perform durable admission and exclusive dispatch claim.
class SearchTimerNativeCasLocalProvider final : public ISearchTimerNativeCasProvider {
public:
    SearchTimerNativeCasLocalProvider(
        std::string localBackendId,
        vdrsuite::agent::SuiteBridgeLocalControlTransport& transport);

    bool supportsAtomicGenerationCasV1() const override;
    SearchTimerNativeCasReply compareAndWrite(
        const SearchTimerNativeCasRequest& request) override;
private:
    std::string localBackendId_;
    vdrsuite::agent::SuiteBridgeLocalControlTransport& transport_;
};
