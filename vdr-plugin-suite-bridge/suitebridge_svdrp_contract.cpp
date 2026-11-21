#include "suitebridge_svdrp_contract.h"

#include "suitebridge_status_snapshot.h"

#include <strings.h>

namespace {

constexpr const char *INVALID_OPTION_MESSAGE =
    "SNAP does not accept options";
constexpr const char *PAYLOAD_UNAVAILABLE_MESSAGE =
    "Suite bridge payload unavailable";

}

SuiteBridgeSvdrpReply::SuiteBridgeSvdrpReply(
    const char *command,
    const char *option,
    unsigned int capabilitySchema,
    const SuiteBridgeStatusSnapshot &snapshot) noexcept
    : kind_(SuiteBridgeSvdrpReplyKind::Unhandled),
      replyCode_(0),
      payload_(capabilitySchema, snapshot)
{
  if (command == nullptr || strcasecmp(command, CommandName()) != 0) {
    return;
  }

  if (option != nullptr && *option != '\0') {
    kind_ = SuiteBridgeSvdrpReplyKind::InvalidOption;
    replyCode_ = InvalidOptionReplyCode();
    return;
  }

  if (!payload_.Complete()) {
    kind_ = SuiteBridgeSvdrpReplyKind::PayloadUnavailable;
    replyCode_ = PayloadUnavailableReplyCode();
    return;
  }

  kind_ = SuiteBridgeSvdrpReplyKind::Payload;
  replyCode_ = SuccessReplyCode();
}

bool SuiteBridgeSvdrpReply::Handled() const noexcept
{
  return kind_ != SuiteBridgeSvdrpReplyKind::Unhandled;
}

bool SuiteBridgeSvdrpReply::HasPayload() const noexcept
{
  return kind_ == SuiteBridgeSvdrpReplyKind::Payload;
}

int SuiteBridgeSvdrpReply::ReplyCode() const noexcept
{
  return replyCode_;
}

const char *SuiteBridgeSvdrpReply::Data() const noexcept
{
  switch (kind_) {
    case SuiteBridgeSvdrpReplyKind::Payload:
      return payload_.Data();
    case SuiteBridgeSvdrpReplyKind::InvalidOption:
      return INVALID_OPTION_MESSAGE;
    case SuiteBridgeSvdrpReplyKind::PayloadUnavailable:
      return PAYLOAD_UNAVAILABLE_MESSAGE;
    case SuiteBridgeSvdrpReplyKind::Unhandled:
      break;
  }

  return nullptr;
}

std::size_t SuiteBridgeSvdrpReply::Size() const noexcept
{
  switch (kind_) {
    case SuiteBridgeSvdrpReplyKind::Payload:
      return payload_.Size();
    case SuiteBridgeSvdrpReplyKind::InvalidOption:
      return sizeof("SNAP does not accept options") - 1;
    case SuiteBridgeSvdrpReplyKind::PayloadUnavailable:
      return sizeof("Suite bridge payload unavailable") - 1;
    case SuiteBridgeSvdrpReplyKind::Unhandled:
      break;
  }

  return 0;
}
