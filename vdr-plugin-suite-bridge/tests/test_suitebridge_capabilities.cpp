#include "suitebridge_capabilities.h"

#include <array>
#include <cassert>
#include <cstring>

int main()
{
  const auto &capabilities = SuiteBridgeCapabilities::All();

  assert(SuiteBridgeCapabilities::SchemaVersion() == 1);
  assert(capabilities.size() == 6);

  const std::array<const char *, 6> expectedIds = {{
      "lifecycle",
      "status-events",
      "snapshots",
      "local-contract",
      "recording-metadata",
      "mutations",
  }};

  const std::array<SuiteBridgeCapabilityState, 6> expectedStates = {{
      SuiteBridgeCapabilityState::Available,
      SuiteBridgeCapabilityState::Available,
      SuiteBridgeCapabilityState::Available,
      SuiteBridgeCapabilityState::Available,
      SuiteBridgeCapabilityState::Available,
      SuiteBridgeCapabilityState::Disabled,
  }};

  for (std::size_t index = 0; index < capabilities.size(); ++index) {
    assert(std::strcmp(capabilities[index].id, expectedIds[index]) == 0);
    assert(capabilities[index].state == expectedStates[index]);
    assert(SuiteBridgeCapabilities::Find(expectedIds[index]) ==
        &capabilities[index]);
  }

  assert(std::strcmp(
      SuiteBridgeCapabilities::StateName(
          SuiteBridgeCapabilityState::Available),
      "available") == 0);
  assert(std::strcmp(
      SuiteBridgeCapabilities::StateName(
          SuiteBridgeCapabilityState::Planned),
      "planned") == 0);
  assert(std::strcmp(
      SuiteBridgeCapabilities::StateName(
          SuiteBridgeCapabilityState::Disabled),
      "disabled") == 0);

  assert(SuiteBridgeCapabilities::IsAvailable("lifecycle"));
  assert(SuiteBridgeCapabilities::IsAvailable("status-events"));
  assert(SuiteBridgeCapabilities::IsAvailable("snapshots"));
  assert(SuiteBridgeCapabilities::IsAvailable("local-contract"));
  assert(SuiteBridgeCapabilities::IsAvailable("recording-metadata"));
  assert(!SuiteBridgeCapabilities::IsAvailable("mutations"));
  assert(!SuiteBridgeCapabilities::IsAvailable("unknown"));
  assert(SuiteBridgeCapabilities::Find(nullptr) == nullptr);

  return 0;
}
