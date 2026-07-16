#ifndef VDR_SUITE_BRIDGE_STATUS_SNAPSHOT_H
#define VDR_SUITE_BRIDGE_STATUS_SNAPSHOT_H

class SuiteBridgeStatusSnapshot final {
public:
  SuiteBridgeStatusSnapshot(
      bool monitorActive,
      unsigned long long channelSwitchCount,
      unsigned long long recordingCount,
      unsigned long long replayingCount,
      unsigned long long timerChangeCount) noexcept;

  SuiteBridgeStatusSnapshot(const SuiteBridgeStatusSnapshot &) noexcept = default;
  SuiteBridgeStatusSnapshot &operator=(const SuiteBridgeStatusSnapshot &) = delete;

  static constexpr unsigned int SchemaVersion() noexcept
  {
    return 1;
  }

  bool MonitorActive() const noexcept;
  unsigned long long ChannelSwitchCount() const noexcept;
  unsigned long long RecordingCount() const noexcept;
  unsigned long long ReplayingCount() const noexcept;
  unsigned long long TimerChangeCount() const noexcept;
  unsigned long long TotalCount() const noexcept;

private:
  const bool monitorActive_;
  const unsigned long long channelSwitchCount_;
  const unsigned long long recordingCount_;
  const unsigned long long replayingCount_;
  const unsigned long long timerChangeCount_;
};

#endif
