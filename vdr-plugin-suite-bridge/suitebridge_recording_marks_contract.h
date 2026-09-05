#ifndef VDR_SUITE_BRIDGE_RECORDING_MARKS_CONTRACT_H
#define VDR_SUITE_BRIDGE_RECORDING_MARKS_CONTRACT_H

#include "suitebridge_recording_marks.h"

#include <array>
#include <cstddef>
#include <string>

class SuiteBridgeRecordingMarksRequest final {
public:
  SuiteBridgeRecordingMarksRequest(
      const char *command,
      const char *option);

  bool Handled() const noexcept;
  bool Valid() const noexcept;
  const std::string &RecordingKey() const noexcept;

private:
  bool handled_ = false;
  bool valid_ = false;
  std::string recordingKey_;
};

class SuiteBridgeRecordingMarksPayload final {
public:
  explicit SuiteBridgeRecordingMarksPayload(
      const SuiteBridgeRecordingMarks &marks);

  const char *Data() const noexcept;
  std::size_t Size() const noexcept;
  bool Complete() const noexcept;

private:
  static constexpr std::size_t kCapacity =
      SuiteBridgeRecordingMarks::kMaximumPayloadBytes + 1;

  std::array<char, kCapacity> data_{};
  std::size_t size_ = 0;
  bool complete_ = false;
};

#endif
