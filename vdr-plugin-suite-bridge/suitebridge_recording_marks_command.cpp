#include "suitebridge_recording_marks_command.h"

#include "suitebridge_plugin_identity.h"
#include "suitebridge_recording_identity.h"
#include "suitebridge_recording_marks.h"
#include "suitebridge_recording_marks_contract.h"

#include <vdr/recording.h>
#include <vdr/tools.h>

#include <cstddef>
#include <string>
#include <utility>

namespace {
constexpr int kRecordingReadLockTimeoutMs = 500;
}

SuiteBridgeCommandResult SuiteBridgeRecordingMarksCommand::Handle(
    const char *command,
    const char *option)
{
  const SuiteBridgeRecordingMarksRequest request(command, option);
  if (!request.Handled()) {
    return {};
  }

  SuiteBridgeCommandResult result;
  result.handled = true;
  if (!request.Valid()) {
    result.replyCode = 501;
    result.payload = std::string("Usage: PLUG ") +
        SuiteBridgePluginIdentity::Name + " RMARKS <recording-key>";
    return result;
  }

  const std::string recordingKey = request.RecordingKey();
  SuiteBridgeRecordingMarks response;
  response.recordingKey = recordingKey;
  std::size_t matchCount = 0;
  std::string nativeFileName;

  {
    cStateKey stateKey;
    const cRecordings *recordings = cRecordings::GetRecordingsRead(
        stateKey, kRecordingReadLockTimeoutMs);
    if (!recordings) {
      result.replyCode = 451;
      result.payload = stateKey.TimedOut()
          ? "Recording list read lock timed out"
          : "Recording list read lock unavailable";
      esyslog("suitebridge: svdrp command=RMARKS result=recordings-lock-unavailable recording_key=%s",
          recordingKey.c_str());
      return result;
    }

    // Keep the native recording pointer entirely within the list lock.
    // Copy the values needed for the read-only marks operation before release.
    for (const cRecording *recording = recordings->First();
         recording != nullptr;
         recording = recordings->Next(recording)) {
      const char *nativeId = recording->FileName();
      if (!nativeId ||
          SuiteBridgeRecordingIdentity::KeyForNativeId(nativeId) != recordingKey) {
        continue;
      }
      ++matchCount;
      if (matchCount > 1) {
        break;
      }
      nativeFileName = nativeId;
      response.framesPerSecond = recording->FramesPerSecond();
      response.isPesRecording = recording->IsPesRecording();
      response.inUseFlags = recording->IsInUse();
      response.marksFilePresent = recording->HasMarks();
    }
    stateKey.Remove();
  }

  if (matchCount == 0) {
    response.reason = SuiteBridgeRecordingMarksReason::RecordingNotFound;
  } else if (matchCount > 1) {
    response.reason = SuiteBridgeRecordingMarksReason::IdentityAmbiguous;
  } else {
    response.found = true;
    response.reason = SuiteBridgeRecordingMarksReason::None;
    if (response.framesPerSecond <= 0.0) {
      response.state = SuiteBridgeRecordingMarksState::Unreadable;
    } else if (!response.marksFilePresent) {
      response.state = SuiteBridgeRecordingMarksState::None;
      response.marksRevision = SuiteBridgeRecordingMarksRevision(response);
    } else {
      // File I/O and mark serialization must not hold the VDR recordings lock.
      cMarks nativeMarks;
      if (!nativeMarks.Load(
              nativeFileName.c_str(),
              response.framesPerSecond,
              response.isPesRecording)) {
        response.state = SuiteBridgeRecordingMarksState::Unreadable;
      } else {
        response.sequenceCount = nativeMarks.GetNumSequences();
        response.state = nativeMarks.Count() > 0
            ? SuiteBridgeRecordingMarksState::Present
            : SuiteBridgeRecordingMarksState::None;

        for (cMark *mark = nativeMarks.First();
             mark != nullptr;
             mark = nativeMarks.Next(mark)) {
          if (response.marks.size() >= SuiteBridgeRecordingMarks::kMaxMarks) {
            result.replyCode = 451;
            result.payload = "Recording marks exceed contract capacity";
            return result;
          }
          SuiteBridgeRecordingMark nativeMark;
          nativeMark.positionFrame = mark->Position();
          const cString timecode = IndexToHMSF(
              nativeMark.positionFrame,
              true,
              response.framesPerSecond);
          nativeMark.timecode = *timecode ? *timecode : "";
          nativeMark.comment = mark->Comment() ? mark->Comment() : "";
          if (nativeMark.comment.size() >
              SuiteBridgeRecordingMarks::kMaxCommentBytes) {
            result.replyCode = 451;
            result.payload = "Recording mark comment exceeds contract capacity";
            return result;
          }
          response.marks.push_back(std::move(nativeMark));
        }
        response.marksRevision = SuiteBridgeRecordingMarksRevision(response);
      }
    }
  }

  try {
    const SuiteBridgeRecordingMarksPayload payload(response);
    if (!payload.Complete()) {
      result.replyCode = 451;
      result.payload = "Recording marks payload exceeds contract capacity";
      esyslog(
          "suitebridge: svdrp command=RMARKS result=overflow recording_key=%s",
          recordingKey.c_str());
      return result;
    }

    result.replyCode = 250;
    result.payload.assign(payload.Data(), payload.Size());
    isyslog(
        "suitebridge: svdrp command=RMARKS result=served recording_key=%s found=%s state=%s marks=%zu sequences=%d in_use=%d",
        recordingKey.c_str(),
        response.found ? "true" : "false",
        SuiteBridgeRecordingMarksStateName(response.state),
        response.marks.size(),
        response.sequenceCount,
        response.inUseFlags);
    return result;
  } catch (...) {
    result.replyCode = 451;
    result.payload = "Recording marks payload serialization failed";
    esyslog(
        "suitebridge: svdrp command=RMARKS result=serialization-failed recording_key=%s",
        recordingKey.c_str());
    return result;
  }
}
