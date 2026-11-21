#ifndef VDR_SUITE_BRIDGE_SVDRP_CONTRACT_H
#define VDR_SUITE_BRIDGE_SVDRP_CONTRACT_H

#include "suitebridge_local_contract.h"

#include <cstddef>

class SuiteBridgeStatusSnapshot;

enum class SuiteBridgeSvdrpReplyKind {
  Unhandled,
  Payload,
  InvalidOption,
  PayloadUnavailable,
};

class SuiteBridgeSvdrpReply final {
public:
  SuiteBridgeSvdrpReply(
      const char *command,
      const char *option,
      unsigned int capabilitySchema,
      const SuiteBridgeStatusSnapshot &snapshot) noexcept;

  SuiteBridgeSvdrpReply(const SuiteBridgeSvdrpReply &) noexcept = default;
  SuiteBridgeSvdrpReply &operator=(const SuiteBridgeSvdrpReply &) = delete;

  static constexpr const char *CommandName() noexcept
  {
    return "SNAP";
  }

  static constexpr int SuccessReplyCode() noexcept
  {
    return 900;
  }

  static constexpr int InvalidOptionReplyCode() noexcept
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
  SuiteBridgeSvdrpReplyKind kind_;
  int replyCode_;
  SuiteBridgeLocalContractPayload payload_;
};

#endif
