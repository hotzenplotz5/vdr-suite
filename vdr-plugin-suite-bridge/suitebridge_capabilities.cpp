#include "suitebridge_capabilities.h"

#include <cstring>

namespace {

constexpr std::array<SuiteBridgeCapabilityDescriptor, 8> CAPABILITIES = {{
    {"lifecycle", SuiteBridgeCapabilityState::Available},
    {"status-events", SuiteBridgeCapabilityState::Available},
    {"snapshots", SuiteBridgeCapabilityState::Available},
    {"local-contract", SuiteBridgeCapabilityState::Available},
    {"recording-metadata", SuiteBridgeCapabilityState::Available},
    {"epg-type-snapshot", SuiteBridgeCapabilityState::Available},
    {"vdr.live.stream", SuiteBridgeCapabilityState::Available},
    {"mutations", SuiteBridgeCapabilityState::Disabled},
}};

}

const std::array<SuiteBridgeCapabilityDescriptor, 8> &
SuiteBridgeCapabilities::All() noexcept
{
  return CAPABILITIES;
}

const char *SuiteBridgeCapabilities::StateName(
    SuiteBridgeCapabilityState state) noexcept
{
  switch (state) {
    case SuiteBridgeCapabilityState::Available:
      return "available";
    case SuiteBridgeCapabilityState::Planned:
      return "planned";
    case SuiteBridgeCapabilityState::Disabled:
      return "disabled";
  }

  return "unknown";
}

const SuiteBridgeCapabilityDescriptor *SuiteBridgeCapabilities::Find(
    const char *id) noexcept
{
  if (id == nullptr) {
    return nullptr;
  }

  for (const auto &capability : CAPABILITIES) {
    if (std::strcmp(capability.id, id) == 0) {
      return &capability;
    }
  }

  return nullptr;
}

bool SuiteBridgeCapabilities::IsAvailable(const char *id) noexcept
{
  const SuiteBridgeCapabilityDescriptor *capability = Find(id);
  return capability != nullptr &&
      capability->state == SuiteBridgeCapabilityState::Available;
}
