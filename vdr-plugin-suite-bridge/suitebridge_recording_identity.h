#ifndef VDR_SUITE_BRIDGE_RECORDING_IDENTITY_H
#define VDR_SUITE_BRIDGE_RECORDING_IDENTITY_H

#include <cstddef>
#include <string>

class SuiteBridgeRecordingIdentity final {
public:
  static constexpr std::size_t kMaximumNativeIdBytes = 4096;
  static constexpr std::size_t kKeyBytes = 32;

  static std::string KeyForNativeId(const std::string &nativeId);
  static bool IsValidKey(const std::string &key) noexcept;
};

#endif
