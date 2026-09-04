#include "suitebridge_recording_marks_command.h"

#include "suitebridge_plugin_identity.h"
#include "suitebridge_recording_identity.h"
#include "suitebridge_recording_marks.h"
#include "suitebridge_recording_marks_contract.h"

#include <vdr/recording.h>
#include <vdr/tools.h>

#include <cstddef>
#include <string>

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

  {
    LOCK_RECORDINGS_READ;
    const cRecording *matchedRecording = nullptr;
    for (const cRecording *recording = Recordings->First();
         recording != nullptr;
         recording = Recordings->Next(recording)) {
      const char *nativeId = recording->FileName();
      if (!nativeId ||
          SuiteBridgeRecordingIdentity::KeyForNativeId(nativeId) != recordingKey) {
        continue;
      }
      matchedRecording = recording;
      ++matchCount;
      if (matchCount > 1) {
        break;
      }
    }

    if (matchCount == 1 && matchedRecording) {
      response.found = true;
      response.reason = SuiteBridgeRecordingMarksReason::None;
      response.framesPerSecond = matchedRecording->FramesPerSecond();
      response.isPesRecording = matchedRecording->IsPesRecording();
      response.inUseFlags = matchedRecording->IsInUse();
      response.marksFilePresent = matchedRecording->HasMarks();

      if (response.framesPerSecond <= 0.0) {
        response.state = SuiteBridgeRecordingMarksState::Unreadable;
      } else if (!response.marksFilePresent) {
        response.state = SuiteBridgeRecordingMarksState::None;
        response.marksRevision = SuiteBridgeRecordingMarksRevision(response);
      } else {
        cMarks nativeMarks;
        if (!nativeMarks.Load(
                matchedRecording->FileName(),
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
  }

  if (matchCount == 0) {
    response.reason = SuiteBridgeRecordingMarksReason::RecordingNotFound;
  } else if (matchCount > 1) {
    response.reason = SuiteBridgeRecordingMarksReason::IdentityAmbiguous;
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
