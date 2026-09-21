#ifndef VDR_SUITE_BRIDGE_OSD_SNAPSHOT_CONTRACT_H
#define VDR_SUITE_BRIDGE_OSD_SNAPSHOT_CONTRACT_H

#include "suitebridge_osd_snapshot.h"

#include <cstddef>
#include <string>

class SuiteBridgeOsdSnapshotPayload final {
public:
  static constexpr unsigned int SchemaVersion() noexcept { return 1; }
  static constexpr std::size_t MaximumPayloadBytes = 131000;

  explicit SuiteBridgeOsdSnapshotPayload(
      const SuiteBridgeOsdSnapshot &snapshot) noexcept;

  const char *Data() const noexcept;
  std::size_t Size() const noexcept;
  bool Complete() const noexcept;

private:
  bool Append(const std::string &value) noexcept;
  bool AppendJsonString(const char *value) noexcept;
  bool AppendUnsigned(unsigned long long value) noexcept;
  bool AppendSigned(long long value) noexcept;

  std::string data_;
  bool complete_ = true;
};

#endif
