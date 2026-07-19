#include "suitebridge_capability_discovery.h"

#include "suitebridge_capabilities.h"
#include "suitebridge_local_contract.h"
#include "suitebridge_status_snapshot.h"

#include <cstdarg>
#include <cstdio>
#include <limits>

#include <strings.h>

namespace {

constexpr const char *MALFORMED_OPTION_MESSAGE =
    "CAPS requires a decimal discovery schema";
constexpr const char *UNSUPPORTED_SCHEMA_MESSAGE =
    "CAPS discovery schema unsupported";
constexpr const char *PAYLOAD_UNAVAILABLE_MESSAGE =
    "Suite bridge capability payload unavailable";

enum class RequestedSchemaKind {
  Current,
  Supported,
  Unsupported,
  Malformed,
};

bool IsAsciiSpace(unsigned char value) noexcept
{
  return value == ' ' ||
      value == '\t' ||
      value == '\n' ||
      value == '\r' ||
      value == '\v' ||
      value == '\f';
}

bool IsAsciiDigit(unsigned char value) noexcept
{
  return value >= '0' && value <= '9';
}

RequestedSchemaKind ParseRequestedSchema(const char *option) noexcept
{
  if (option == nullptr) {
    return RequestedSchemaKind::Current;
  }

  const unsigned char *cursor =
      reinterpret_cast<const unsigned char *>(option);

  while (*cursor != '\0' && IsAsciiSpace(*cursor)) {
    ++cursor;
  }

  if (*cursor == '\0') {
    return RequestedSchemaKind::Current;
  }

  unsigned long long value = 0;
  bool hasDigit = false;
  bool overflow = false;
  constexpr unsigned long long maximum =
      std::numeric_limits<unsigned long long>::max();

  while (*cursor != '\0' && IsAsciiDigit(*cursor)) {
    hasDigit = true;
    const unsigned int digit = static_cast<unsigned int>(*cursor - '0');

    if (value > (maximum - digit) / 10) {
      overflow = true;
    } else if (!overflow) {
      value = value * 10 + digit;
    }

    ++cursor;
  }

  if (!hasDigit) {
    return RequestedSchemaKind::Malformed;
  }

  while (*cursor != '\0' && IsAsciiSpace(*cursor)) {
    ++cursor;
  }

  if (*cursor != '\0') {
    return RequestedSchemaKind::Malformed;
  }

  if (overflow || value != SuiteBridgeCapabilityDiscoveryPayload::SchemaVersion()) {
    return RequestedSchemaKind::Unsupported;
  }

  return RequestedSchemaKind::Supported;
}

}

SuiteBridgeCapabilityDiscoveryPayload::SuiteBridgeCapabilityDiscoveryPayload(
    const char *pluginName,
    const char *pluginVersion) noexcept
    : data_{{0}},
      size_(0),
      complete_(true)
{
  if (pluginName == nullptr || pluginVersion == nullptr) {
    complete_ = false;
    return;
  }

  if (!Append(
          "{\"discovery_schema\":%u,\"plugin_name\":\"%s\",\"plugin_version\":\"%s\",\"capability_schema\":%u,\"snapshot_schema\":%u,\"local_contract_schema\":%u,\"capabilities\":[",
          SchemaVersion(),
          pluginName,
          pluginVersion,
          SuiteBridgeCapabilities::SchemaVersion(),
          SuiteBridgeStatusSnapshot::SchemaVersion(),
          SuiteBridgeLocalContractPayload::SchemaVersion())) {
    return;
  }

  const auto &capabilities = SuiteBridgeCapabilities::All();

  for (std::size_t index = 0; index < capabilities.size(); ++index) {
    const auto &capability = capabilities[index];

    if (!Append(
            "%s{\"id\":\"%s\",\"state\":\"%s\"}",
            index == 0 ? "" : ",",
            capability.id,
            SuiteBridgeCapabilities::StateName(capability.state))) {
      return;
    }
  }

  (void)Append("]}");
}

bool SuiteBridgeCapabilityDiscoveryPayload::Append(
    const char *format,
    ...) noexcept
{
  if (!complete_ || format == nullptr || size_ >= data_.size()) {
    complete_ = false;
    return false;
  }

  va_list arguments;
  va_start(arguments, format);

  const int written = std::vsnprintf(
      data_.data() + size_,
      data_.size() - size_,
      format,
      arguments);

  va_end(arguments);

  if (written < 0) {
    data_[0] = '\0';
    size_ = 0;
    complete_ = false;
    return false;
  }

  const std::size_t requestedSize = static_cast<std::size_t>(written);

  if (requestedSize >= data_.size() - size_) {
    data_.back() = '\0';
    size_ = data_.size() - 1;
    complete_ = false;
    return false;
  }

  size_ += requestedSize;
  return true;
}

const char *SuiteBridgeCapabilityDiscoveryPayload::Data() const noexcept
{
  return data_.data();
}

std::size_t SuiteBridgeCapabilityDiscoveryPayload::Size() const noexcept
{
  return size_;
}

bool SuiteBridgeCapabilityDiscoveryPayload::Complete() const noexcept
{
  return complete_;
}

SuiteBridgeCapabilityDiscoveryReply::SuiteBridgeCapabilityDiscoveryReply(
    const char *command,
    const char *option,
    const char *pluginName,
    const char *pluginVersion) noexcept
    : kind_(SuiteBridgeCapabilityDiscoveryReplyKind::Unhandled),
      replyCode_(0),
      payload_(pluginName, pluginVersion)
{
  if (command == nullptr || strcasecmp(command, CommandName()) != 0) {
    return;
  }

  const RequestedSchemaKind requestedSchema = ParseRequestedSchema(option);

  if (requestedSchema == RequestedSchemaKind::Malformed) {
    kind_ = SuiteBridgeCapabilityDiscoveryReplyKind::MalformedOption;
    replyCode_ = MalformedOptionReplyCode();
    return;
  }

  if (requestedSchema == RequestedSchemaKind::Unsupported) {
    kind_ = SuiteBridgeCapabilityDiscoveryReplyKind::UnsupportedSchema;
    replyCode_ = UnsupportedSchemaReplyCode();
    return;
  }

  if (!payload_.Complete()) {
    kind_ = SuiteBridgeCapabilityDiscoveryReplyKind::PayloadUnavailable;
    replyCode_ = PayloadUnavailableReplyCode();
    return;
  }

  kind_ = SuiteBridgeCapabilityDiscoveryReplyKind::Payload;
  replyCode_ = SuccessReplyCode();
}

bool SuiteBridgeCapabilityDiscoveryReply::Handled() const noexcept
{
  return kind_ != SuiteBridgeCapabilityDiscoveryReplyKind::Unhandled;
}

bool SuiteBridgeCapabilityDiscoveryReply::HasPayload() const noexcept
{
  return kind_ == SuiteBridgeCapabilityDiscoveryReplyKind::Payload;
}

int SuiteBridgeCapabilityDiscoveryReply::ReplyCode() const noexcept
{
  return replyCode_;
}

const char *SuiteBridgeCapabilityDiscoveryReply::Data() const noexcept
{
  switch (kind_) {
    case SuiteBridgeCapabilityDiscoveryReplyKind::Payload:
      return payload_.Data();
    case SuiteBridgeCapabilityDiscoveryReplyKind::MalformedOption:
      return MALFORMED_OPTION_MESSAGE;
    case SuiteBridgeCapabilityDiscoveryReplyKind::UnsupportedSchema:
      return UNSUPPORTED_SCHEMA_MESSAGE;
    case SuiteBridgeCapabilityDiscoveryReplyKind::PayloadUnavailable:
      return PAYLOAD_UNAVAILABLE_MESSAGE;
    case SuiteBridgeCapabilityDiscoveryReplyKind::Unhandled:
      break;
  }

  return nullptr;
}

std::size_t SuiteBridgeCapabilityDiscoveryReply::Size() const noexcept
{
  switch (kind_) {
    case SuiteBridgeCapabilityDiscoveryReplyKind::Payload:
      return payload_.Size();
    case SuiteBridgeCapabilityDiscoveryReplyKind::MalformedOption:
      return sizeof("CAPS requires a decimal discovery schema") - 1;
    case SuiteBridgeCapabilityDiscoveryReplyKind::UnsupportedSchema:
      return sizeof("CAPS discovery schema unsupported") - 1;
    case SuiteBridgeCapabilityDiscoveryReplyKind::PayloadUnavailable:
      return sizeof("Suite bridge capability payload unavailable") - 1;
    case SuiteBridgeCapabilityDiscoveryReplyKind::Unhandled:
      break;
  }

  return 0;
}
