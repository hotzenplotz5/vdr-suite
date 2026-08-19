#ifndef VDR_SUITE_BRIDGE_CAPABILITIES_H
#define VDR_SUITE_BRIDGE_CAPABILITIES_H

#include <array>

enum class SuiteBridgeCapabilityState {
  Available,
  Planned,
  Disabled,
};

struct SuiteBridgeCapabilityDescriptor {
  const char *id;
  SuiteBridgeCapabilityState state;
};

class SuiteBridgeCapabilities final {
public:
  static constexpr unsigned int SchemaVersion() noexcept
  {
    return 1;
  }

  static const std::array<SuiteBridgeCapabilityDescriptor, 8> &All() noexcept;
  static const char *StateName(SuiteBridgeCapabilityState state) noexcept;
  static const SuiteBridgeCapabilityDescriptor *Find(const char *id) noexcept;
  static bool IsAvailable(const char *id) noexcept;
};

#endif
