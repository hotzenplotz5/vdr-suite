#pragma once
#include "PinnedMediaImportAgentExecutor.h"
#include "BackendAgentCommandClient.h"
#include <functional>
#include <string>
class IBackendAgentControlPlaneTransport;

namespace vdrsuite::agent {
// Revalidates the existing Core grant/operation/agent receipt fence.
// This is not a parallel local grant store and does not authorize by manifest.
class ReceiptRevalidatingMediaImportAuthorization final
    : public vdrsuite::mediatools::ILocalMediaImportAuthorization {
public:
    using CurrentAgentContext = std::function<BackendAgentCommandClientContext()>;
    ReceiptRevalidatingMediaImportAuthorization(
        std::string commandStatePath, IBackendAgentControlPlaneTransport& transport,
        CurrentAgentContext currentContext);
    bool granted(const BackendAgentCommandAssignment& assignment) override;
private:
    std::string statePath_;
    IBackendAgentControlPlaneTransport& transport_;
    CurrentAgentContext currentContext_;
};
} // namespace vdrsuite::agent
