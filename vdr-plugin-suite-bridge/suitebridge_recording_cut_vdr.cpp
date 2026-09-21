#include "suitebridge_recording_cut_vdr.h"

#include "suitebridge_plugin_identity.h"
#include "suitebridge_recording_cut_state_command.h"
#include "suitebridge_recording_identity.h"
#include "suitebridge_recording_marks.h"

#include <vdr/cutter.h>
#include <vdr/recording.h>
#include <vdr/tools.h>

#include <cstddef>
#include <cstring>
#include <sstream>
#include <string>
#include <strings.h>
#include <unistd.h>

namespace {
SuiteBridgeRecordingCutMutationResult result(
    SuiteBridgeRecordingCutDisposition disposition,
    const char *reason,
    const SuiteBridgeRecordingCutRequest &request)
{
  SuiteBridgeRecordingCutMutationResult value;
  value.disposition = disposition;
  value.evidenceReference =
      std::string("ncut:vdr:") + reason + ':' + request.commandId;
  return value;
}

SuiteBridgeRecordingCutMutationResult queued(
    const std::string &editedRecordingKey,
    const SuiteBridgeRecordingCutRequest &request)
{
  SuiteBridgeRecordingCutMutationResult value;
  value.disposition = SuiteBridgeRecordingCutDisposition::AcceptedUnverified;
  value.evidenceReference = std::string("ncut:vdr:queued:") +
      editedRecordingKey + ':' + request.commandId;
  return value;
}

SuiteBridgeRecordingCutMutationResult rejected(
    const char *reason,
    const SuiteBridgeRecordingCutRequest &request)
{
  return result(
      SuiteBridgeRecordingCutDisposition::RejectedWithoutEffect,
      reason,
      request);
}

SuiteBridgeRecordingCutMutationResult unknown(
    const char *reason,
    const SuiteBridgeRecordingCutRequest &request)
{
  return result(
      SuiteBridgeRecordingCutDisposition::OutcomeUnknown,
      reason,
      request);
}

bool loadCurrentMarks(
    const cRecording &recording,
    const std::string &recordingKey,
    cMarks &nativeMarks,
    SuiteBridgeRecordingMarks &snapshot)
{
  snapshot.found = true;
  snapshot.recordingKey = recordingKey;
  snapshot.framesPerSecond = recording.FramesPerSecond();
  snapshot.isPesRecording = recording.IsPesRecording();
  snapshot.inUseFlags = recording.IsInUse();
  snapshot.marksFilePresent = recording.HasMarks();
  if (snapshot.framesPerSecond <= 0.0) return false;
  if (!snapshot.marksFilePresent) {
    snapshot.state = SuiteBridgeRecordingMarksState::None;
    snapshot.marksRevision = SuiteBridgeRecordingMarksRevision(snapshot);
    return SuiteBridgeRecordingMarksRevisionValid(snapshot.marksRevision);
  }

  if (!nativeMarks.Load(
          recording.FileName(),
          snapshot.framesPerSecond,
          snapshot.isPesRecording)) {
    return false;
  }
  snapshot.sequenceCount = nativeMarks.GetNumSequences();
  snapshot.state = nativeMarks.Count() > 0
      ? SuiteBridgeRecordingMarksState::Present
      : SuiteBridgeRecordingMarksState::None;
  for (cMark *mark = nativeMarks.First();
       mark != nullptr;
       mark = nativeMarks.Next(mark)) {
    if (snapshot.marks.size() >= SuiteBridgeRecordingMarks::kMaxMarks)
      return false;
    SuiteBridgeRecordingMark item;
    item.positionFrame = mark->Position();
    item.comment = mark->Comment() ? mark->Comment() : "";
    if (item.comment.size() > SuiteBridgeRecordingMarks::kMaxCommentBytes)
      return false;
    snapshot.marks.push_back(item);
  }
  snapshot.marksRevision = SuiteBridgeRecordingMarksRevision(snapshot);
  return SuiteBridgeRecordingMarksRevisionValid(snapshot.marksRevision);
}

SuiteBridgeRecordingCutState inspectLocked(
    const cRecordings *recordings,
    const std::string &recordingKey,
    const cRecording *&matchedRecording)
{
  SuiteBridgeRecordingCutState state;
  state.recordingKey = recordingKey;
  matchedRecording = nullptr;

  std::size_t matchCount = 0;
  for (const cRecording *candidate = recordings->First();
       candidate != nullptr;
       candidate = recordings->Next(candidate)) {
    const char *nativeId = candidate->FileName();
    if (!nativeId ||
        SuiteBridgeRecordingIdentity::KeyForNativeId(nativeId) != recordingKey) {
      continue;
    }
    matchedRecording = candidate;
    ++matchCount;
    if (matchCount > 1) break;
  }

  if (matchCount == 0 || matchedRecording == nullptr) {
    state.reason = "recording-not-found";
    matchedRecording = nullptr;
    return state;
  }
  if (matchCount > 1) {
    state.reason = "recording-identity-ambiguous";
    matchedRecording = nullptr;
    return state;
  }

  state.found = true;
  state.inUseFlags = matchedRecording->IsInUse();
  state.handlerUsage = static_cast<int>(
      RecordingsHandler.GetUsage(matchedRecording->FileName()));

  cMarks nativeMarks;
  SuiteBridgeRecordingMarks marks;
  state.marksReadable = loadCurrentMarks(
      *matchedRecording, recordingKey, nativeMarks, marks);
  if (state.marksReadable) {
    state.marksRevision = marks.marksRevision;
    state.marksFilePresent = marks.marksFilePresent;
    state.markCount = nativeMarks.Count();
    state.sequenceCount = nativeMarks.GetNumSequences();
  }

  const cString editedFileName =
      cCutter::EditedFileName(matchedRecording->FileName());
  const char *edited = *editedFileName;
  const bool editedAvailable = edited != nullptr && *edited != '\0';
  const bool editedSame = editedAvailable &&
      std::strcmp(edited, matchedRecording->FileName()) == 0;
  bool editedIdentityValid = false;
  if (editedAvailable && !editedSame) {
    state.editedRecordingKey =
        SuiteBridgeRecordingIdentity::KeyForNativeId(edited);
    editedIdentityValid =
        SuiteBridgeRecordingIdentity::IsValidKey(state.editedRecordingKey);
    state.editedDestinationExists = access(edited, F_OK) == 0;
    if (editedIdentityValid) {
      for (const cRecording *candidate = recordings->First();
           candidate != nullptr;
           candidate = recordings->Next(candidate)) {
        const char *nativeId = candidate->FileName();
        if (nativeId != nullptr && std::strcmp(nativeId, edited) == 0 &&
            SuiteBridgeRecordingIdentity::KeyForNativeId(nativeId) ==
                state.editedRecordingKey) {
          state.editedRecordingFound = true;
          break;
        }
      }
    }
  }

  if (state.inUseFlags != 0) {
    state.reason = "recording-in-use";
  } else if (!state.marksReadable) {
    state.reason = "marks-unreadable";
  } else if (!state.marksFilePresent || state.markCount == 0) {
    state.reason = "no-marks";
  } else if (state.sequenceCount <= 0) {
    state.reason = "invalid-cut-sequence";
  } else if (state.handlerUsage != static_cast<int>(ruNone)) {
    state.reason = "recording-handler-busy";
  } else if (!editedAvailable) {
    state.reason = "edited-destination-unavailable";
  } else if (editedSame) {
    state.reason = "edited-destination-invalid";
  } else if (!editedIdentityValid) {
    state.reason = "edited-destination-identity-invalid";
  } else if (state.editedDestinationExists) {
    state.reason = "edited-destination-exists";
  } else {
    state.reason = "ready";
    state.ready = true;
  }

  return state;
}

const char *flag(bool value) noexcept
{
  return value ? "1" : "0";
}
} // namespace

SuiteBridgeRecordingCutState
SuiteBridgeRecordingCutVdrMutationCallback::Inspect(
    const std::string &recordingKey) const
{
  LOCK_RECORDINGS_READ;
  const cRecording *recording = nullptr;
  return inspectLocked(Recordings, recordingKey, recording);
}

SuiteBridgeCommandResult SuiteBridgeRecordingCutStateCommand::Handle(
    const char *command,
    const char *option)
{
  if (command == nullptr || strcasecmp(command, "RCUT") != 0) return {};

  SuiteBridgeCommandResult response;
  response.handled = true;
  const std::string recordingKey = option ? option : "";
  if (!SuiteBridgeRecordingIdentity::IsValidKey(recordingKey)) {
    response.replyCode = 501;
    response.payload = std::string("Usage: PLUG ") +
        SuiteBridgePluginIdentity::Name + " RCUT <recording-key>";
    return response;
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

    response.replyCode = 250;
    response.payload = payload.str();
    isyslog(
        "suitebridge: svdrp command=RCUT result=served recording_key=%s found=%s reason=%s ready=%s handler_usage=%d edited_found=%s",
        recordingKey.c_str(),
        state.found ? "true" : "false",
        state.reason.c_str(),
        state.ready ? "true" : "false",
        state.handlerUsage,
        state.editedRecordingFound ? "true" : "false");
    return response;
  } catch (...) {
    response.replyCode = 451;
    response.payload = "Recording cut state unavailable";
    esyslog(
        "suitebridge: svdrp command=RCUT result=unavailable recording_key=%s",
        recordingKey.c_str());
    return response;
  }
}

SuiteBridgeRecordingCutMutationResult
SuiteBridgeRecordingCutVdrMutationCallback::StartCut(
    const SuiteBridgeRecordingCutRequest &request)
{
  try {
    LOCK_RECORDINGS_READ;
    const cRecording *recording = nullptr;
    SuiteBridgeRecordingCutState current =
        inspectLocked(Recordings, request.recordingKey, recording);
    if (!current.found || recording == nullptr)
      return rejected(current.reason.c_str(), request);
    if (!current.marksReadable)
      return rejected(current.reason.c_str(), request);
    if (current.marksRevision != request.expectedMarksRevision)
      return rejected("marks-revision-mismatch", request);
    if (!current.ready)
      return rejected(current.reason.c_str(), request);

    const cRecording *finalRecording = nullptr;
    SuiteBridgeRecordingCutState finalState =
        inspectLocked(Recordings, request.recordingKey, finalRecording);
    if (!finalState.found || finalRecording == nullptr)
      return rejected(finalState.reason.c_str(), request);
    if (!finalState.marksReadable)
      return rejected(finalState.reason.c_str(), request);
    if (finalState.marksRevision != request.expectedMarksRevision)
      return rejected("marks-revision-mismatch", request);
    if (!finalState.ready)
      return rejected(finalState.reason.c_str(), request);
    if (finalState.editedRecordingKey != current.editedRecordingKey ||
        !SuiteBridgeRecordingIdentity::IsValidKey(
            finalState.editedRecordingKey))
      return rejected("edited-destination-changed", request);

    if (!RecordingsHandler.Add(ruCut, finalRecording->FileName()))
      return rejected("cut-queue-rejected", request);

    return queued(finalState.editedRecordingKey, request);
  } catch (...) {
    return unknown("exception", request);
  }
}
