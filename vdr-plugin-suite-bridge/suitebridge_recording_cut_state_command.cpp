#include "suitebridge_recording_cut_state_command.h"

#include "suitebridge_plugin_identity.h"
#include "suitebridge_recording_cut_state.h"
#include "suitebridge_recording_cut_vdr.h"
#include "suitebridge_recording_identity.h"

#include <vdr/tools.h>

#include <sstream>
#include <string>
#include <strings.h>

namespace {
const char *flag(bool value) noexcept
{
  return value ? "1" : "0";
}
}

SuiteBridgeCommandResult SuiteBridgeRecordingCutStateCommand::Handle(
    const char *command,
    const char *option)
{
  if (command == nullptr || strcasecmp(command, "RCUT") != 0) return {};

  SuiteBridgeCommandResult result;
  result.handled = true;
  const std::string recordingKey = option ? option : "";
  if (!SuiteBridgeRecordingIdentity::IsValidKey(recordingKey)) {
    result.replyCode = 501;
    result.payload = std::string("Usage: PLUG ") +
        SuiteBridgePluginIdentity::Name + " RCUT <recording-key>";
    return result;
  }

  try {
    SuiteBridgeRecordingCutVdrMutationCallback inspector;
    const SuiteBridgeRecordingCutState state = inspector.Inspect(recordingKey);
    const std::string marksRevision =
        state.marksRevision.empty() ? "none" : state.marksRevision;
    const std::string editedRecordingKey =
        state.editedRecordingKey.empty() ? "none" : state.editedRecordingKey;

    std::ostringstream payload;
    payload << SuiteBridgeRecordingCutState::Protocol << ' '
            << state.recordingKey << ' '
            << flag(state.found) << ' '
            << state.reason << ' '
            << flag(state.ready) << ' '
            << flag(state.marksReadable) << ' '
            << marksRevision << ' '
            << flag(state.marksFilePresent) << ' '
            << state.markCount << ' '
            << state.sequenceCount << ' '
            << state.inUseFlags << ' '
            << state.handlerUsage << ' '
            << editedRecordingKey << ' '
            << flag(state.editedDestinationExists) << ' '
            << flag(state.editedRecordingFound);

    result.replyCode = 250;
    result.payload = payload.str();
    isyslog(
        "suitebridge: svdrp command=RCUT result=served recording_key=%s found=%s reason=%s ready=%s handler_usage=%d edited_found=%s",
        recordingKey.c_str(),
        state.found ? "true" : "false",
        state.reason.c_str(),
        state.ready ? "true" : "false",
        state.handlerUsage,
        state.editedRecordingFound ? "true" : "false");
    return result;
  } catch (...) {
    result.replyCode = 451;
    result.payload = "Recording cut state unavailable";
    esyslog(
        "suitebridge: svdrp command=RCUT result=unavailable recording_key=%s",
        recordingKey.c_str());
    return result;
  }
}
