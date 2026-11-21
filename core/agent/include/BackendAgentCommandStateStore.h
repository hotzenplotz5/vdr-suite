#pragma once

#include "BackendAgentCommand.h"
#include "BackendAgentCommandStateExtension.h"

#include <cstdint>
#include <string>

namespace vdrsuite::agent::commandstate
{

struct LocalState
{
    ::BackendAgentCommandAssignment assignment;
    ::BackendAgentCommandReceipt receipt;
    ::BackendAgentCommandResult result;
    bool receiptAcknowledged = false;
    bool resultPresent = false;
    bool resultAcknowledged = false;
    std::string dispatchState = "not_started";
    std::string nativeCapabilityEvidence;
    std::string pluginInstanceEpoch;
    std::string probeNonce;
    std::uint64_t nativeExecutionSequence = 0;
    std::string nativeReceiptEvidence;
    std::string nativeResultEvidence;
    std::string nativeReadbackEvidence;
    bool stateExtensionPresent = false;
    BackendAgentCommandStateExtension stateExtension;
};

bool load(const std::string& path, LocalState& state, std::string& reason);

bool persist(
    const std::string& path,
    const LocalState& state,
    std::string& reason);

bool retireProtectedState(const std::string& path, std::string& reason);

}
