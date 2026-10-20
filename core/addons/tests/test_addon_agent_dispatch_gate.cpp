#include "AddonAgentDispatchGate.h"
#include <cassert>
#include <string>

int main()
{
    using namespace vdrsuite::addons;
    const std::string digest(64, 'a');
    HandlerInstallFacts facts;
    facts.origin = InstallOrigin::manual;
    facts.metadataPresent = true;
    facts.integrityMatches = true;
    facts.sourceApproved = true;
    facts.ownershipSecure = true;
    facts.versionCompatible = true;
    facts.handlerHealthy = true;
    ProviderBinding observed{"rectools", "vdr-suite-addon-media-tools",
        "backend-a", digest, digest, "0.1.0", "0.1.0", 5, 5, facts};
    ProviderApproval approved{"rectools", "vdr-suite-addon-media-tools",
        "backend-a", digest, "0.1.0", InstallOrigin::manual, 5, true};
    vdrsuite::operations::MutationOperation op;
    op.operationId = "operation-a";
    op.backendId = "backend-a";
    op.backendGeneration = 5;
    BackendAgentCommandAssignment command;
    command.present = true;
    command.commandId = "command-a";
    command.operationId = "operation-a";
    command.backendId = "backend-a";
    command.backendGeneration = 5;
    auto result = previewMediaImportDispatch(op, command, approved, observed);
    assert(!result.permitted && result.reason == DispatchGateReason::unsupported);
    command.backendGeneration = 6;
    assert(previewMediaImportDispatch(op, command, approved, observed).reason ==
           DispatchGateReason::staleGeneration);
    command.backendGeneration = 5;
    command.operationId = "wrong";
    assert(previewMediaImportDispatch(op, command, approved, observed).reason ==
           DispatchGateReason::mismatchedOperation);
    command.operationId = "operation-a";
    approved.independentlyAuthorized = false;
    assert(previewMediaImportDispatch(op, command, approved, observed).reason ==
           DispatchGateReason::approvalUnavailable);
    approved.independentlyAuthorized = true;
    command.present = false;
    assert(previewMediaImportDispatch(op, command, approved, observed).reason ==
           DispatchGateReason::invalidAssignment);
    command.present = true;
    op.backendGeneration = 0;
    assert(previewMediaImportDispatch(op, command, approved, observed).reason ==
           DispatchGateReason::invalidOperation);
}
