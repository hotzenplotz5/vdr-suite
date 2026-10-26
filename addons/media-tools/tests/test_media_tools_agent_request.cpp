#include "MediaToolsAgentRequest.h"
#include <cassert>

int main()
{
    using namespace vdrsuite::mediatools;
    AgentImportEnvelope request{
        "rectools", "vdr-suite-addon-media-tools",
        "addon.media.import", "op-1", "cmd-1", "backend-a",
        5, 5, 200, 150
    };
    assert(inspectImportEnvelope(request, 100) ==
           ImportEnvelopeReason::candidateOnly);
    request.commandType = "addon.media.shrink";
    assert(inspectImportEnvelope(request, 100) ==
           ImportEnvelopeReason::invalidIdentity);
    request.commandType = "addon.media.import";
    request.agentGeneration = 6;
    assert(inspectImportEnvelope(request, 100) ==
           ImportEnvelopeReason::staleGeneration);
    request.agentGeneration = 5;
    request.commandDeadline = 50;
    assert(inspectImportEnvelope(request, 100) ==
           ImportEnvelopeReason::invalidDeadline);
    request.commandDeadline = 150;
    request.backendId = "*";
    assert(inspectImportEnvelope(request, 100) ==
           ImportEnvelopeReason::missingOperation);
}
