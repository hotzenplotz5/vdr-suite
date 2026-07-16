#ifndef VDR_SUITE_BRIDGE_STATUS_SNAPSHOT_H
#define VDR_SUITE_BRIDGE_STATUS_SNAPSHOT_H

#include <array>
#include <cstddef>

class SuiteBridgeStatusSnapshot final {
public:
  static constexpr std::size_t CounterEpochLength() noexcept
  {
    return 32;
  }

  SuiteBridgeStatusSnapshot(
      bool monitorActive,
      unsigned long long channelSwitchCount,
      unsigned long long recordingCount,
      unsigned long long replayingCount,
      unsigned long long timerChangeCount,
      const char *counterEpoch,
      bool counterOverflow) noexcept;

  SuiteBridgeStatusSnapshot(const SuiteBridgeStatusSnapshot &) noexcept = default;
  SuiteBridgeStatusSnapshot &operator=(const SuiteBridgeStatusSnapshot &) = delete;

  static constexpr unsigned int SchemaVersion() noexcept
  {
    return 2;
  }

  bool MonitorActive() const noexcept;
  unsigned long long ChannelSwitchCount() const noexcept;
  unsigned long long RecordingCount() const noexcept;
  unsigned long long ReplayingCount() const noexcept;
  unsigned long long TimerChangeCount() const noexcept;
  unsigned long long TotalCount() const noexcept;
  const char *CounterEpoch() const noexcept;
  bool CounterOverflow() const noexcept;

private:
  bool monitorActive_;
  unsigned long long channelSwitchCount_;
  unsigned long long recordingCount_;
  unsigned long long replayingCount_;
  unsigned long long timerChangeCount_;
  unsigned long long totalCount_;
  std::array<char, CounterEpochLength() + 1> counterEpoch_;
  bool counterOverflow_;
};

#endif
