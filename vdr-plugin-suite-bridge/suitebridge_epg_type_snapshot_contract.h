#ifndef VDR_SUITE_BRIDGE_EPG_TYPE_SNAPSHOT_CONTRACT_H
#define VDR_SUITE_BRIDGE_EPG_TYPE_SNAPSHOT_CONTRACT_H

#include "suitebridge_epg_metadata.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

struct SuiteBridgeEpgTypeSnapshotItem final {
  std::string channelId;
  unsigned int eventId = 0;
  std::int64_t startTime = 0;
  std::int64_t endTime = 0;
  SuiteBridgeEpgMediaType mediaType = SuiteBridgeEpgMediaType::None;
};

struct SuiteBridgeEpgTypeSnapshotPage final {
  std::uint64_t nextOffset = 0;
  std::size_t scanned = 0;
  bool done = true;
  std::vector<SuiteBridgeEpgTypeSnapshotItem> items;
};

class SuiteBridgeEpgTypeSnapshotRequest final {
public:
  SuiteBridgeEpgTypeSnapshotRequest(const char *command, const char *option);

  bool Handled() const noexcept;
  bool Valid() const noexcept;
  std::int64_t FromTime() const noexcept;
  std::int64_t UntilTime() const noexcept;
  std::uint64_t Offset() const noexcept;
  std::size_t Limit() const noexcept;

private:
  bool handled_ = false;
  bool valid_ = false;
  std::int64_t fromTime_ = 0;
  std::int64_t untilTime_ = 0;
  std::uint64_t offset_ = 0;
  std::size_t limit_ = 0;
};

class SuiteBridgeEpgTypeSnapshotPayload final {
public:
  explicit SuiteBridgeEpgTypeSnapshotPayload(
      const SuiteBridgeEpgTypeSnapshotPage &page);

  const char *Data() const noexcept;
  std::size_t Size() const noexcept;
  bool Complete() const noexcept;

private:
  // Keep enough room for SVDRP framing below the agent's 8192-byte bound.
  static constexpr std::size_t kCapacity = 7680;

  std::array<char, kCapacity> data_{};
  std::size_t size_ = 0;
  bool complete_ = false;
};

#endif
