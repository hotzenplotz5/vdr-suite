#pragma once

#include "BackendAgentCommandClient.h"
#include <functional>
#include <string>

class IBackendAgentControlPlaneTransport;

namespace vdrsuite::agent {

// Real Core receipt/grant revalidation immediately before HEVC stages and
// publications. Separate from local candidate validity and path resolution.
// Uses the existing authenticated Agent transport; it never mints grants or
// stores a second authority table. No offline execution is permitted.
class ReceiptRevalidatingMediaShrinkAuthorization final {
public:
    using CurrentContext = std::function<BackendAgentCommandClientContext()>;
    ReceiptRevalidatingMediaShrinkAuthorization(
        std::string commandStatePath,
        IBackendAgentControlPlaneTransport& transport,
        CurrentContext context);

    bool granted(const BackendAgentCommandAssignment& assignment);

private:
    std::string statePath_;
    IBackendAgentControlPlaneTransport& transport_;
    CurrentContext context_;
};
} // namespace vdrsuite::agent
