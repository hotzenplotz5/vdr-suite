#include "suitebridge_status_snapshot.h"

SuiteBridgeStatusSnapshot::SuiteBridgeStatusSnapshot(
    bool monitorActive,
    unsigned long long channelSwitchCount,
    unsigned long long recordingCount,
    unsigned long long replayingCount,
    unsigned long long timerChangeCount) noexcept
    : monitorActive_(monitorActive),
      channelSwitchCount_(channelSwitchCount),
      recordingCount_(recordingCount),
      replayingCount_(replayingCount),
      timerChangeCount_(timerChangeCount)
{
}

bool SuiteBridgeStatusSnapshot::MonitorActive() const noexcept
{
  return monitorActive_;
}

unsigned long long SuiteBridgeStatusSnapshot::ChannelSwitchCount() const noexcept
{
  return channelSwitchCount_;
}

unsigned long long SuiteBridgeStatusSnapshot::RecordingCount() const noexcept
{
  return recordingCount_;
}

unsigned long long SuiteBridgeStatusSnapshot::ReplayingCount() const noexcept
{
  return replayingCount_;
}

unsigned long long SuiteBridgeStatusSnapshot::TimerChangeCount() const noexcept
{
  return timerChangeCount_;
}

unsigned long long SuiteBridgeStatusSnapshot::TotalCount() const noexcept
{
  return channelSwitchCount_ + recordingCount_ + replayingCount_ +
      timerChangeCount_;
}
