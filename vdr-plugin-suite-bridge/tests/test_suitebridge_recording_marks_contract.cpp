#include "suitebridge_recording_identity.h"
#include "suitebridge_recording_marks.h"
#include "suitebridge_recording_marks_contract.h"

#include <cassert>
#include <string>

namespace {

SuiteBridgeRecordingMarks makeEmptyMarks()
{
  SuiteBridgeRecordingMarks marks;
  marks.found = true;
  marks.recordingKey = SuiteBridgeRecordingIdentity::KeyForNativeId(
      "/srv/vdr/video/Test/2026-09-04.08.00.1-0.rec");
  marks.state = SuiteBridgeRecordingMarksState::None;
  marks.framesPerSecond = 25.0;
  marks.isPesRecording = false;
  marks.inUseFlags = 0;
  marks.marksFilePresent = false;
  marks.sequenceCount = 0;
  marks.marksRevision = SuiteBridgeRecordingMarksRevision(marks);
  return marks;
}

} // namespace

int main()
{
  const SuiteBridgeRecordingMarks empty = makeEmptyMarks();
  assert(SuiteBridgeRecordingMarksRevisionValid(empty.marksRevision));

  SuiteBridgeRecordingMarksRequest request(
      "rmarks", empty.recordingKey.c_str());
  assert(request.Handled());
  assert(request.Valid());
  assert(request.RecordingKey() == empty.recordingKey);

  SuiteBridgeRecordingMarksRequest extra(
      "RMARKS", (empty.recordingKey + " extra").c_str());
  assert(extra.Handled());
  assert(!extra.Valid());

  SuiteBridgeRecordingMarksRequest unrelated("RMETA", empty.recordingKey.c_str());
  assert(!unrelated.Handled());

  const SuiteBridgeRecordingMarksPayload emptyPayload(empty);
  assert(emptyPayload.Complete());
  const std::string emptyJson(emptyPayload.Data(), emptyPayload.Size());
  assert(emptyJson.find("\"schema\":1") != std::string::npos);
  assert(emptyJson.find("\"state\":\"none\"") != std::string::npos);
  assert(emptyJson.find("\"marks\":[]") != std::string::npos);
  assert(emptyJson.find(empty.marksRevision) != std::string::npos);

  SuiteBridgeRecordingMarks populated = empty;
  populated.state = SuiteBridgeRecordingMarksState::Present;
  populated.marksFilePresent = true;
  populated.sequenceCount = 1;
  populated.marks = {
      {100, "00:00:04.00", "begin"},
      {250, "00:00:10.00", "end \"quoted\""},
  };
  populated.marksRevision = SuiteBridgeRecordingMarksRevision(populated);
  assert(SuiteBridgeRecordingMarksRevisionValid(populated.marksRevision));
  assert(populated.marksRevision != empty.marksRevision);

  const SuiteBridgeRecordingMarksPayload populatedPayload(populated);
  assert(populatedPayload.Complete());
  const std::string populatedJson(
      populatedPayload.Data(), populatedPayload.Size());
  assert(populatedJson.find("\"positionFrame\":100") != std::string::npos);
  assert(populatedJson.find("\"positionSeconds\":4") != std::string::npos);
  assert(populatedJson.find("end \\\"quoted\\\"") != std::string::npos);
  assert(populatedJson.find("\"sequenceCount\":1") != std::string::npos);

  SuiteBridgeRecordingMarks moved = populated;
  moved.marks[0].positionFrame = 101;
  moved.marksRevision = SuiteBridgeRecordingMarksRevision(moved);
  assert(moved.marksRevision != populated.marksRevision);

  SuiteBridgeRecordingMarks unreadable = empty;
  unreadable.state = SuiteBridgeRecordingMarksState::Unreadable;
  unreadable.marksRevision.clear();
  const SuiteBridgeRecordingMarksPayload unreadablePayload(unreadable);
  assert(unreadablePayload.Complete());
  const std::string unreadableJson(
      unreadablePayload.Data(), unreadablePayload.Size());
  assert(unreadableJson.find("\"state\":\"unreadable\"") != std::string::npos);

  SuiteBridgeRecordingMarks invalid = populated;
  invalid.marks[1].positionFrame = 50;
  invalid.marksRevision = SuiteBridgeRecordingMarksRevision(invalid);
  const SuiteBridgeRecordingMarksPayload invalidPayload(invalid);
  assert(!invalidPayload.Complete());

  SuiteBridgeRecordingMarks missing;
  missing.recordingKey = empty.recordingKey;
  missing.reason = SuiteBridgeRecordingMarksReason::RecordingNotFound;
  const SuiteBridgeRecordingMarksPayload missingPayload(missing);
  assert(missingPayload.Complete());
  const std::string missingJson(missingPayload.Data(), missingPayload.Size());
  assert(missingJson.find("\"found\":false") != std::string::npos);
  assert(missingJson.find("recording_not_found") != std::string::npos);

  return 0;
}
