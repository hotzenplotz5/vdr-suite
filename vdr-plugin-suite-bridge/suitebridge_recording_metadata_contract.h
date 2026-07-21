#ifndef VDR_SUITE_BRIDGE_RECORDING_METADATA_CONTRACT_H
#define VDR_SUITE_BRIDGE_RECORDING_METADATA_CONTRACT_H

#include "suitebridge_recording_metadata.h"

#include <array>
#include <cstddef>
#include <string>

class SuiteBridgeRecordingMetadataRequest final {
public:
  SuiteBridgeRecordingMetadataRequest(
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

class SuiteBridgeRecordingMetadataPayload final {
public:
  explicit SuiteBridgeRecordingMetadataPayload(
      const SuiteBridgeRecordingMetadata &metadata);

  const char *Data() const noexcept;
  std::size_t Size() const noexcept;
  bool Complete() const noexcept;

private:
  static constexpr std::size_t kCapacity = 7680;

  std::array<char, kCapacity> data_{};
  std::size_t size_ = 0;
  bool complete_ = false;
};

#endif
