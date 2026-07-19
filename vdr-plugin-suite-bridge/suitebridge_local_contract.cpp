#include "suitebridge_local_contract.h"

#include "suitebridge_status_snapshot.h"

#include <cstdio>

SuiteBridgeLocalContractPayload::SuiteBridgeLocalContractPayload(
    unsigned int capabilitySchema,
    const SuiteBridgeStatusSnapshot &snapshot) noexcept
    : data_{{0}},
      size_(0),
      complete_(false)
{
  const int written = std::snprintf(
      data_.data(),
      data_.size(),
      "{\"contract_schema\":%u,\"capability_schema\":%u,\"snapshot_schema\":%u,\"active\":%s,\"total\":%llu,\"channel_switch\":%llu,\"recording\":%llu,\"replaying\":%llu,\"timer_change\":%llu,\"counter_epoch\":\"%s\",\"counter_overflow\":%s}",
      SchemaVersion(),
      capabilitySchema,
      SuiteBridgeStatusSnapshot::SchemaVersion(),
      snapshot.MonitorActive() ? "true" : "false",
      snapshot.TotalCount(),
      snapshot.ChannelSwitchCount(),
      snapshot.RecordingCount(),
      snapshot.ReplayingCount(),
      snapshot.TimerChangeCount(),
      snapshot.CounterEpoch(),
      snapshot.CounterOverflow() ? "true" : "false");

  if (written < 0) {
    data_[0] = '\0';
    return;
  }

  const std::size_t requestedSize = static_cast<std::size_t>(written);

  if (requestedSize >= data_.size()) {
    data_.back() = '\0';
    size_ = data_.size() - 1;
    return;
  }

  size_ = requestedSize;
  complete_ = true;
}

const char *SuiteBridgeLocalContractPayload::Data() const noexcept
{
  return data_.data();
}

std::size_t SuiteBridgeLocalContractPayload::Size() const noexcept
{
  return size_;
}

bool SuiteBridgeLocalContractPayload::Complete() const noexcept
{
  return complete_;
}
