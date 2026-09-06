#include "suitebridge_recording_cut_vdr.h"

#include "suitebridge_recording_identity.h"
#include "suitebridge_recording_marks.h"

#include <vdr/cutter.h>
#include <vdr/recording.h>

#include <cstddef>
#include <cstring>
#include <string>
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
} // namespace

SuiteBridgeRecordingCutMutationResult
SuiteBridgeRecordingCutVdrMutationCallback::StartCut(
    const SuiteBridgeRecordingCutRequest &request)
{
  try {
    LOCK_RECORDINGS_READ;
    const cRecording *recording = nullptr;
    std::size_t matchCount = 0;
    for (const cRecording *candidate = Recordings->First();
         candidate != nullptr;
         candidate = Recordings->Next(candidate)) {
      const char *nativeId = candidate->FileName();
      if (!nativeId ||
          SuiteBridgeRecordingIdentity::KeyForNativeId(nativeId) !=
              request.recordingKey) {
        continue;
      }
      recording = candidate;
      ++matchCount;
      if (matchCount > 1) break;
    }

    if (matchCount == 0 || recording == nullptr)
      return rejected("recording-not-found", request);
    if (matchCount > 1)
      return rejected("recording-identity-ambiguous", request);
    if (recording->IsInUse() != 0)
      return rejected("recording-in-use", request);

    cMarks nativeMarks;
    SuiteBridgeRecordingMarks current;
    if (!loadCurrentMarks(*recording, request.recordingKey, nativeMarks, current))
      return rejected("marks-unreadable", request);
    if (current.marksRevision != request.expectedMarksRevision)
      return rejected("marks-revision-mismatch", request);
    if (!current.marksFilePresent || nativeMarks.Count() == 0)
      return rejected("no-marks", request);
    if (nativeMarks.GetNumSequences() <= 0)
      return rejected("invalid-cut-sequence", request);
    if (RecordingsHandler.GetUsage(recording->FileName()) != ruNone)
      return rejected("recording-handler-busy", request);

    const cString editedFileName =
        cCutter::EditedFileName(recording->FileName());
    const char *edited = *editedFileName;
    if (edited == nullptr || *edited == '\0')
      return rejected("edited-destination-unavailable", request);
    if (std::strcmp(edited, recording->FileName()) == 0)
      return rejected("edited-destination-invalid", request);
    if (access(edited, F_OK) == 0)
      return rejected("edited-destination-exists", request);
    const std::string editedRecordingKey =
        SuiteBridgeRecordingIdentity::KeyForNativeId(edited);
    if (!SuiteBridgeRecordingIdentity::IsValidKey(editedRecordingKey))
      return rejected("edited-destination-identity-invalid", request);

    cMarks finalMarks;
    SuiteBridgeRecordingMarks finalSnapshot;
    if (!loadCurrentMarks(
            *recording, request.recordingKey, finalMarks, finalSnapshot))
      return rejected("marks-unreadable", request);
    if (finalSnapshot.marksRevision != request.expectedMarksRevision)
      return rejected("marks-revision-mismatch", request);
    if (recording->IsInUse() != 0 ||
        RecordingsHandler.GetUsage(recording->FileName()) != ruNone)
      return rejected("recording-in-use", request);

    if (!RecordingsHandler.Add(ruCut, recording->FileName()))
      return rejected("cut-queue-rejected", request);

    return queued(editedRecordingKey, request);
  } catch (...) {
    return unknown("exception", request);
  }
}
