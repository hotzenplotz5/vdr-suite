#pragma once

#include "BackendAgentCommand.h"
#include "BackendAgentNativeTimerModify.h"

namespace vdrsuite::agent
{
enum class BackendAgentNativeTimerModifyLocalPhase { starting, completed };
enum class BackendAgentNativeTimerModifyRecoveryDecision {
    failClosed, reconcileOnly, returnPersistedEvidence
};
struct BackendAgentNativeTimerModifyLocalState {
    std::uint64_t schemaVersion=1;
    BackendAgentNativeTimerModifyLocalPhase phase=
        BackendAgentNativeTimerModifyLocalPhase::starting;
    BackendAgentNativeTimerModifyCommand command;
    std::int64_t localStartingPersistedAt=0;
    BackendAgentNativeTimerModifyEvidence evidence;
};
struct BackendAgentNativeTimerModifyRecoveryResult {
    BackendAgentNativeTimerModifyRecoveryDecision decision=
        BackendAgentNativeTimerModifyRecoveryDecision::failClosed;
    std::string reasonCode;
    BackendAgentNativeTimerModifyEvidence evidence;
};
// Canonical Agent result body. The result's dispatch timestamps originate
// in the durable local modification evidence, never the control-plane clock.
std::string backendAgentNativeTimerModifyResultEvidence(
    const BackendAgentNativeTimerModifyEvidence& evidence,
    const BackendAgentCommandAssignment& assignment,
    std::string& reason);

bool backendAgentNativeTimerModifyParseResultEvidence(
    const std::string& encoded,
    const BackendAgentCommandAssignment& assignment,
    BackendAgentNativeTimerModifyEvidence& evidence,
    std::string& reason);

bool backendAgentNativeTimerModifyCommandFromAssignment(
    const BackendAgentCommandAssignment&,BackendAgentNativeTimerModifyCommand&,
    std::string&);
bool backendAgentNativeTimerModifyPrepareLocalStarting(
    const BackendAgentCommandAssignment&,std::int64_t,
    BackendAgentNativeTimerModifyLocalState&,std::string&);
bool backendAgentNativeTimerModifyLocalStateValid(
    const BackendAgentNativeTimerModifyLocalState&,std::string&);
bool backendAgentNativeTimerModifyCompleteLocalState(
    BackendAgentNativeTimerModifyLocalState&,
    const BackendAgentNativeTimerModifyEvidence&,std::string&);
BackendAgentNativeTimerModifyRecoveryResult backendAgentNativeTimerModifyRecoverLocalState(
    const BackendAgentNativeTimerModifyLocalState&,const std::string&,
    const std::string&,const std::string&,std::uint64_t,std::int64_t);
std::string backendAgentNativeTimerModifySerializeLocalState(
    const BackendAgentNativeTimerModifyLocalState&,std::string&);
bool backendAgentNativeTimerModifyParseLocalState(
    const std::string&,BackendAgentNativeTimerModifyLocalState&,std::string&);
}
