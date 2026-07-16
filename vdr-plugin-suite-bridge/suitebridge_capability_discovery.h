#ifndef VDR_SUITE_BRIDGE_CAPABILITY_DISCOVERY_H
#define VDR_SUITE_BRIDGE_CAPABILITY_DISCOVERY_H

#include <array>
#include <cstddef>

class SuiteBridgeCapabilityDiscoveryPayload final {
public:
  static constexpr unsigned int SchemaVersion() noexcept
  {
    return 1;
  }

  static constexpr std::size_t BufferSize = 768;

  SuiteBridgeCapabilityDiscoveryPayload(
      const char *pluginName,
      const char *pluginVersion) noexcept;

  SuiteBridgeCapabilityDiscoveryPayload(
      const SuiteBridgeCapabilityDiscoveryPayload &) noexcept = default;
  SuiteBridgeCapabilityDiscoveryPayload &operator=(
      const SuiteBridgeCapabilityDiscoveryPayload &) = delete;

  const char *Data() const noexcept;
  std::size_t Size() const noexcept;
  bool Complete() const noexcept;

private:
  bool Append(const char *format, ...) noexcept;

  std::array<char, BufferSize> data_;
  std::size_t size_;
  bool complete_;
};

enum class SuiteBridgeCapabilityDiscoveryReplyKind {
  Unhandled,
  Payload,
  MalformedOption,
  UnsupportedSchema,
  PayloadUnavailable,
};

class SuiteBridgeCapabilityDiscoveryReply final {
public:
  SuiteBridgeCapabilityDiscoveryReply(
      const char *command,
      const char *option,
      const char *pluginName,
      const char *pluginVersion) noexcept;

  SuiteBridgeCapabilityDiscoveryReply(
      const SuiteBridgeCapabilityDiscoveryReply &) noexcept = default;
  SuiteBridgeCapabilityDiscoveryReply &operator=(
      const SuiteBridgeCapabilityDiscoveryReply &) = delete;

  static constexpr const char *CommandName() noexcept
  {
    return "CAPS";
  }

  static constexpr int SuccessReplyCode() noexcept
  {
    return 900;
  }

  static constexpr int MalformedOptionReplyCode() noexcept
  {
    return 501;
  }

  static constexpr int UnsupportedSchemaReplyCode() noexcept
  {
    return 504;
  }

  static constexpr int PayloadUnavailableReplyCode() noexcept
  {
    return 451;
  }

  bool Handled() const noexcept;
  bool HasPayload() const noexcept;
  int ReplyCode() const noexcept;
  const char *Data() const noexcept;
  std::size_t Size() const noexcept;

private:
  SuiteBridgeCapabilityDiscoveryReplyKind kind_;
  int replyCode_;
  SuiteBridgeCapabilityDiscoveryPayload payload_;
};

#endif
