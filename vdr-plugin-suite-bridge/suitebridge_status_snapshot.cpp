#include "suitebridge_status_snapshot.h"

#include <limits>

namespace {

void AddSaturating(
    unsigned long long value,
    unsigned long long &total,
    bool &overflow) noexcept
{
  constexpr unsigned long long maximum =
      std::numeric_limits<unsigned long long>::max();

  if (maximum - total < value) {
    total = maximum;
    overflow = true;
    return;
  }

  total += value;
}

}

SuiteBridgeStatusSnapshot::SuiteBridgeStatusSnapshot(
    bool monitorActive,
    unsigned long long channelSwitchCount,
    unsigned long long recordingCount,
    unsigned long long replayingCount,
    unsigned long long timerChangeCount,
    const char *counterEpoch,
    bool counterOverflow) noexcept
    : monitorActive_(monitorActive),
      channelSwitchCount_(channelSwitchCount),
      recordingCount_(recordingCount),
      replayingCount_(replayingCount),
      timerChangeCount_(timerChangeCount),
      totalCount_(0),
      counterEpoch_{{0}},
      counterOverflow_(counterOverflow)
{
  AddSaturating(channelSwitchCount_, totalCount_, counterOverflow_);
  AddSaturating(recordingCount_, totalCount_, counterOverflow_);
  AddSaturating(replayingCount_, totalCount_, counterOverflow_);
  AddSaturating(timerChangeCount_, totalCount_, counterOverflow_);

  if (counterEpoch == nullptr) {
    counterOverflow_ = true;
    return;
  }

  for (std::size_t index = 0; index < CounterEpochLength(); ++index) {
    const char character = counterEpoch[index];

    if (character == '\0') {
      counterOverflow_ = true;
      return;
    }

    counterEpoch_[index] = character;
  }

  counterEpoch_[CounterEpochLength()] = '\0';

  if (counterEpoch[CounterEpochLength()] != '\0') {
    counterOverflow_ = true;
  }
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
  return totalCount_;
}

const char *SuiteBridgeStatusSnapshot::CounterEpoch() const noexcept
{
  return counterEpoch_.data();
}

bool SuiteBridgeStatusSnapshot::CounterOverflow() const noexcept
{
  return counterOverflow_;
}
