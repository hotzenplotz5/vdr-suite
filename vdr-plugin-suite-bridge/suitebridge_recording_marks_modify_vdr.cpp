#include "suitebridge_recording_marks_modify_vdr.h"

#include "suitebridge_recording_identity.h"
#include "suitebridge_recording_marks.h"

#include <vdr/recording.h>
#include <vdr/status.h>

#include <algorithm>
#include <cstddef>
#include <string>
#include <vector>

namespace {
SuiteBridgeRecordingMarksModifyMutationResult result(
    SuiteBridgeRecordingMarksModifyMutationDisposition disposition,
    const char *reason,
    const SuiteBridgeRecordingMarksModifyRequest &request)
{
  SuiteBridgeRecordingMarksModifyMutationResult value;
  value.disposition = disposition;
  value.evidenceReference =
      std::string("nmarks:vdr:") + reason + ':' + request.commandId;
  return value;
}

SuiteBridgeRecordingMarksModifyMutationResult rejected(
    const char *reason,
    const SuiteBridgeRecordingMarksModifyRequest &request)
{
  return result(
      SuiteBridgeRecordingMarksModifyMutationDisposition::RejectedWithoutEffect,
      reason,
      request);
}

SuiteBridgeRecordingMarksModifyMutationResult unknown(
    const char *reason,
    const SuiteBridgeRecordingMarksModifyRequest &request)
{
  return result(
      SuiteBridgeRecordingMarksModifyMutationDisposition::OutcomeUnknown,
      reason,
      request);
}

SuiteBridgeRecordingMarksModifyMutationResult applied(
    const SuiteBridgeRecordingMarksModifyRequest &request)
{
  return result(
      SuiteBridgeRecordingMarksModifyMutationDisposition::AppliedUnverified,
      "modified-unverified",
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

  const bool loaded = nativeMarks.Load(
      recording.FileName(),
      snapshot.framesPerSecond,
      snapshot.isPesRecording);
  if (snapshot.marksFilePresent && !loaded) return false;

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

cMark *uniqueMarkAt(cMarks &marks, int frame, int &count)
{
  cMark *matched = nullptr;
  count = 0;
  for (cMark *mark = marks.First(); mark != nullptr; mark = marks.Next(mark)) {
    if (mark->Position() != frame) continue;
    matched = mark;
    ++count;
  }
  return matched;
}

bool alignTarget(cIndexFile &indexFile, int requestedFrame, int &alignedFrame)
{
  alignedFrame = indexFile.GetClosestIFrame(requestedFrame);
  return alignedFrame >= 0;
}

bool alignTarget(const cRecording &recording, int requestedFrame, int &alignedFrame)
{
  cIndexFile indexFile(
      recording.FileName(),
      false,
      recording.IsPesRecording());
  return indexFile.Ok() && alignTarget(indexFile, requestedFrame, alignedFrame);
}

bool saveAndNotify(cMarks &marks)
{
  if (!marks.Save()) return false;
  cStatus::MsgMarksModified(&marks);
  return true;
}

std::vector<int> currentFrames(cMarks &marks)
{
  std::vector<int> frames;
  for (cMark *mark = marks.First(); mark != nullptr; mark = marks.Next(mark))
    frames.push_back(mark->Position());
  return frames;
}
} // namespace

SuiteBridgeRecordingMarksModifyMutationResult
SuiteBridgeRecordingMarksModifyVdrMutationCallback::ModifyMarks(
    const SuiteBridgeRecordingMarksModifyRequest &request)
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
    if (current.inUseFlags != 0)
      return rejected("recording-in-use", request);
    if (current.marksRevision != request.expectedMarksRevision)
      return rejected("marks-revision-mismatch", request);

    switch (request.kind) {
    case SuiteBridgeRecordingMarksModifyKind::Add: {
      int alignedFrame = -1;
      if (!alignTarget(*recording, request.targetFrame, alignedFrame))
        return rejected("recording-index-unavailable", request);
      if (nativeMarks.Get(alignedFrame) != nullptr)
        return rejected("mark-already-present", request);
      nativeMarks.Add(alignedFrame);
      if (!saveAndNotify(nativeMarks))
        return unknown("marks-save-failed", request);
      return applied(request);
    }

    case SuiteBridgeRecordingMarksModifyKind::Delete: {
      int sourceCount = 0;
      cMark *source = uniqueMarkAt(nativeMarks, request.sourceFrame, sourceCount);
      if (sourceCount == 0 || source == nullptr)
        return rejected("mark-not-found", request);
      if (sourceCount > 1)
        return rejected("mark-source-ambiguous", request);
      nativeMarks.Del(source);
      if (!saveAndNotify(nativeMarks))
        return unknown("marks-save-failed", request);
      return applied(request);
    }

    case SuiteBridgeRecordingMarksModifyKind::Move: {
      int sourceCount = 0;
      cMark *source = uniqueMarkAt(nativeMarks, request.sourceFrame, sourceCount);
      if (sourceCount == 0 || source == nullptr)
        return rejected("mark-not-found", request);
      if (sourceCount > 1)
        return rejected("mark-source-ambiguous", request);

      int alignedFrame = -1;
      if (!alignTarget(*recording, request.targetFrame, alignedFrame))
        return rejected("recording-index-unavailable", request);
      if (alignedFrame == request.sourceFrame)
        return rejected("mark-position-unchanged", request);

      const cMark *previous = nativeMarks.GetPrev(request.sourceFrame);
      const cMark *next = nativeMarks.GetNext(request.sourceFrame);
      if ((previous && alignedFrame <= previous->Position()) ||
          (next && alignedFrame >= next->Position())) {
        return rejected("mark-neighbor-crossing", request);
      }

      source->SetPosition(alignedFrame);
      nativeMarks.Sort();
      if (!saveAndNotify(nativeMarks))
        return unknown("marks-save-failed", request);
      return applied(request);
    }

    case SuiteBridgeRecordingMarksModifyKind::Reset: {
      if (!recording->HasMarks())
        return rejected("marks-already-reset", request);
      if (!cMarks::DeleteMarksFile(recording))
        return unknown("marks-delete-failed", request);
      cMarks resetMarks;
      resetMarks.Load(
          recording->FileName(),
          recording->FramesPerSecond(),
          recording->IsPesRecording());
      cStatus::MsgMarksModified(&resetMarks);
      return applied(request);
    }

    case SuiteBridgeRecordingMarksModifyKind::Replace: {
      cIndexFile indexFile(
          recording->FileName(),
          false,
          recording->IsPesRecording());
      if (!indexFile.Ok())
        return rejected("recording-index-unavailable", request);

      std::vector<int> alignedFrames;
      alignedFrames.reserve(request.replacementFrames.size());
      for (const int requestedFrame : request.replacementFrames) {
        int alignedFrame = -1;
        if (!alignTarget(indexFile, requestedFrame, alignedFrame))
          return rejected("recording-index-unavailable", request);
        alignedFrames.push_back(alignedFrame);
      }
      std::sort(alignedFrames.begin(), alignedFrames.end());
      if (std::adjacent_find(alignedFrames.begin(), alignedFrames.end()) !=
          alignedFrames.end())
        return rejected("replacement-alignment-collision", request);
      if (alignedFrames == currentFrames(nativeMarks))
        return rejected("marks-replacement-unchanged", request);

      while (cMark *mark = nativeMarks.First()) nativeMarks.Del(mark);
      for (const int frame : alignedFrames) nativeMarks.Add(frame);
      nativeMarks.Sort();
      if (!saveAndNotify(nativeMarks))
        return unknown("marks-save-failed", request);
      return applied(request);
    }
    }
  } catch (...) {
    return unknown("exception", request);
  }

  return unknown("invalid-kind", request);
}
