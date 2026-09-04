#ifndef VDR_SUITE_BRIDGE_RECORDING_MARKS_H
#define VDR_SUITE_BRIDGE_RECORDING_MARKS_H

#include <cstddef>
#include <string>
#include <vector>

enum class SuiteBridgeRecordingMarksReason {
  None,
  RecordingNotFound,
  IdentityAmbiguous,
};

enum class SuiteBridgeRecordingMarksState {
  None,
  Present,
  Unreadable,
};

struct SuiteBridgeRecordingMark final {
  int positionFrame = 0;
  std::string timecode;
  std::string comment;
};

struct SuiteBridgeRecordingMarks final {
  static constexpr std::size_t kMaxMarks = 2048;
  static constexpr std::size_t kMaxCommentBytes = 1024;
  static constexpr std::size_t kMaximumPayloadBytes = 65535;

  bool found = false;
  SuiteBridgeRecordingMarksReason reason = SuiteBridgeRecordingMarksReason::None;
  std::string recordingKey;
  SuiteBridgeRecordingMarksState state = SuiteBridgeRecordingMarksState::None;
  double framesPerSecond = 0.0;
  bool isPesRecording = false;
  int inUseFlags = 0;
  bool marksFilePresent = false;
  int sequenceCount = 0;
  std::string marksRevision;
  std::vector<SuiteBridgeRecordingMark> marks;
};

const char *SuiteBridgeRecordingMarksReasonName(
    SuiteBridgeRecordingMarksReason reason) noexcept;
const char *SuiteBridgeRecordingMarksStateName(
    SuiteBridgeRecordingMarksState state) noexcept;
std::string SuiteBridgeRecordingMarksRevision(
    const SuiteBridgeRecordingMarks &marks);
bool SuiteBridgeRecordingMarksRevisionValid(
    const std::string &revision) noexcept;

#endif
