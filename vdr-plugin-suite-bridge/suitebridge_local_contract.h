#ifndef VDR_SUITE_BRIDGE_LOCAL_CONTRACT_H
#define VDR_SUITE_BRIDGE_LOCAL_CONTRACT_H

#include <array>
#include <cstddef>

class SuiteBridgeStatusSnapshot;

class SuiteBridgeLocalContractPayload final {
public:
  static constexpr unsigned int SchemaVersion() noexcept
  {
    return 1;
  }

  static constexpr std::size_t Capacity() noexcept
  {
    return kCapacity;
  }

  SuiteBridgeLocalContractPayload(
      unsigned int capabilitySchema,
      const SuiteBridgeStatusSnapshot &snapshot) noexcept;

  SuiteBridgeLocalContractPayload(
      const SuiteBridgeLocalContractPayload &) noexcept = default;
  SuiteBridgeLocalContractPayload &operator=(
      const SuiteBridgeLocalContractPayload &) = delete;

  const char *Data() const noexcept;
  std::size_t Size() const noexcept;
  bool Complete() const noexcept;

private:
  static constexpr std::size_t kCapacity = 320;

  std::array<char, kCapacity> data_;
  std::size_t size_;
  bool complete_;
};

#endif
