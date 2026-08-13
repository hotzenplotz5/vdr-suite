#include "suitebridge_native_timer_delete.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <limits>
#include <sstream>
#include <strings.h>
#include <utility>
#include <vector>

namespace {
constexpr const char *NativeOperation = "vdr.timer.delete";
constexpr const char *AuthorityDomain = "vdr.timer";
constexpr const char *ProviderId = "suitebridge:local";
constexpr const char *ProviderKind = "suitebridge";
constexpr const char *CapabilityProtocol = "vdr-suite-ntdel-cap/1";
constexpr const char *ResultProtocol = "vdr-suite-ntdel-result/1";
constexpr std::uint64_t NativeOperationSchema = 1;
constexpr std::uint64_t ProviderGeneration = 1;
constexpr std::uint64_t CapabilityRevision = 1;
constexpr int SuccessReplyCode = 900;
constexpr int MalformedReplyCode = 501;
constexpr int DisabledReplyCode = 556;
constexpr int StaleReplyCode = 555;

bool safeToken(const std::string &value, std::size_t maximum = 512)
{
  return !value.empty() && value.size() <= maximum &&
      std::all_of(value.begin(), value.end(), [](unsigned char character) {
        return std::isalnum(character) != 0 || character == '-' ||
            character == '_' || character == '.' || character == ':';
      });
}

bool unsignedValue(const std::string &value, std::uint64_t &parsed)
{
  if (value.empty() || value.size() > 19) return false;
  parsed = 0;
  for (unsigned char character : value) {
    if (character < '0' || character > '9') return false;
    const unsigned digit = static_cast<unsigned>(character - '0');
    if (parsed >
        (static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max()) - digit) / 10U) {
      return false;
    }
    parsed = parsed * 10U + digit;
  }
  return parsed > 0;
}

std::vector<std::string> split(const char *option)
{
  std::vector<std::string> values;
  if (option == nullptr) return values;
  const std::string input(option);
  std::size_t position = 0;
  while (position < input.size()) {
    while (position < input.size() && input[position] == ' ') ++position;
    if (position == input.size()) break;
    const std::size_t end = input.find(' ', position);
    values.push_back(input.substr(
        position, end == std::string::npos ? std::string::npos : end - position));
    if (values.size() > 32) return {};
    if (end == std::string::npos) break;
    position = end + 1;
  }
  return values;
}

struct Request final {
  std::string commandId;
  std::string requestFingerprint;
  std::string operationId;
  std::string operationRevision;
  std::string nativeTimerBindingId;
  std::string expectedBindingRevision;
  std::string timerAssignmentId;
  std::string backendNativeTimerId;
  std::string jobId;
  std::string attemptId;
  std::uint64_t claimEpoch = 0;
  std::string backendId;
  std::string agentId;
  std::string agentInstanceId;
  std::uint64_t backendGeneration = 0;
  std::uint64_t controlPlaneClaimedAt = 0;
  std::string authorityDomain;
  std::string providerId;
  std::string providerKind;
  std::uint64_t ownershipGeneration = 0;
  std::string providerInstanceEpoch;
  std::uint64_t providerGeneration = 0;
  std::uint64_t capabilityRevision = 0;
  std::string requiredCapability;
  std::uint64_t localStartingPersistedAt = 0;
};

bool parseExecute(const char *option, Request &request)
{
  const std::vector<std::string> values = split(option);
  if (values.size() != 29 || values[0] != "EXEC" ||
      values[1] != "vdr-suite-native/1" ||
      values[2] != NativeOperation || values[3] != "1") {
    return false;
  }

  request.commandId = values[4];
  request.requestFingerprint = values[5];
  request.operationId = values[6];
  request.operationRevision = values[7];
  request.nativeTimerBindingId = values[8];
  request.expectedBindingRevision = values[9];
  request.timerAssignmentId = values[10];
  request.backendNativeTimerId = values[11];
  request.jobId = values[12];
  request.attemptId = values[13];
  request.backendId = values[15];
  request.agentId = values[16];
  request.agentInstanceId = values[17];
  request.authorityDomain = values[20];
  request.providerId = values[21];
  request.providerKind = values[22];
  request.providerInstanceEpoch = values[24];
  request.requiredCapability = values[27];

  return unsignedValue(values[14], request.claimEpoch) &&
      unsignedValue(values[18], request.backendGeneration) &&
      unsignedValue(values[19], request.controlPlaneClaimedAt) &&
      unsignedValue(values[23], request.ownershipGeneration) &&
      unsignedValue(values[25], request.providerGeneration) &&
      unsignedValue(values[26], request.capabilityRevision) &&
      unsignedValue(values[28], request.localStartingPersistedAt) &&
      safeToken(request.commandId, 192) &&
      safeToken(request.requestFingerprint) &&
      safeToken(request.operationId, 192) &&
      safeToken(request.operationRevision, 192) &&
      safeToken(request.nativeTimerBindingId, 192) &&
      safeToken(request.expectedBindingRevision, 192) &&
      safeToken(request.timerAssignmentId, 192) &&
      safeToken(request.backendNativeTimerId, 192) &&
      safeToken(request.jobId, 192) && safeToken(request.attemptId, 192) &&
      safeToken(request.backendId, 192) && safeToken(request.agentId, 192) &&
      safeToken(request.agentInstanceId, 192) &&
      safeToken(request.authorityDomain, 192) &&
      safeToken(request.providerId, 192) && safeToken(request.providerKind, 192) &&
      safeToken(request.providerInstanceEpoch, 192) &&
      safeToken(request.requiredCapability, 192);
}

SuiteBridgeCommandResult genericRejection(int replyCode, const char *reason)
{
  SuiteBridgeCommandResult result;
  result.handled = true;
  result.replyCode = replyCode;
  result.payload = std::string("vdr-suite-ntdel-rejected/1 ") + reason;
  return result;
}

SuiteBridgeCommandResult typedRejection(
    int replyCode,
    const Request &request,
    const std::string &pluginInstanceEpoch,
    const std::string &evidenceReference)
{
  SuiteBridgeCommandResult result;
  result.handled = true;
  result.replyCode = replyCode;
  std::ostringstream payload;
  payload << ResultProtocol << ' ' << request.commandId << ' '
          << request.requestFingerprint << ' ' << NativeOperation << ' '
          << NativeOperationSchema << ' ' << pluginInstanceEpoch << ' '
          << ProviderGeneration << ' ' << CapabilityRevision << ' '
          << "rejected_without_effect disabled " << evidenceReference;
  result.payload = payload.str();
  return result;
}
} // namespace

SuiteBridgeNativeTimerDeleteService::SuiteBridgeNativeTimerDeleteService(
    std::string pluginInstanceEpoch)
    : pluginInstanceEpoch_(std::move(pluginInstanceEpoch))
{
  if (!safeToken(pluginInstanceEpoch_, 192)) pluginInstanceEpoch_ = "pie_invalid";
}

SuiteBridgeCommandResult SuiteBridgeNativeTimerDeleteService::Handle(
    const char *command,
    const char *option) const
{
  if (command == nullptr || strcasecmp(command, "NTDEL") != 0) return {};
  const std::vector<std::string> values = split(option);
  if (values.empty()) return genericRejection(MalformedReplyCode, "malformed");
  if (values.front() == "CAP") return capability(option);
  if (values.front() == "EXEC") return executeDisabled(option);
  return genericRejection(MalformedReplyCode, "unsupported");
}

SuiteBridgeCommandResult SuiteBridgeNativeTimerDeleteService::capability(
    const char *option) const
{
  const std::vector<std::string> values = split(option);
  if (values.size() != 2 || values[0] != "CAP" || values[1] != "1") {
    return genericRejection(MalformedReplyCode, "capability-schema-unsupported");
  }

  SuiteBridgeCommandResult result;
  result.handled = true;
  result.replyCode = SuccessReplyCode;
  std::ostringstream payload;
  payload << CapabilityProtocol << ' ' << NativeOperation << ' '
          << NativeOperationSchema << " timer-delete disabled "
          << ProviderKind << ' ' << pluginInstanceEpoch_ << ' '
          << ProviderGeneration << ' ' << CapabilityRevision << " disabled";
  result.payload = payload.str();
  return result;
}

SuiteBridgeCommandResult SuiteBridgeNativeTimerDeleteService::executeDisabled(
    const char *option) const
{
  Request request;
  if (!parseExecute(option, request)) {
    return genericRejection(MalformedReplyCode, "execute-malformed");
  }

  const bool currentFence =
      request.authorityDomain == AuthorityDomain &&
      request.providerId == ProviderId && request.providerKind == ProviderKind &&
      request.providerInstanceEpoch == pluginInstanceEpoch_ &&
      request.providerGeneration == ProviderGeneration &&
      request.capabilityRevision == CapabilityRevision &&
      request.requiredCapability == NativeOperation &&
      request.localStartingPersistedAt >= request.controlPlaneClaimedAt;

  if (!currentFence) {
    return typedRejection(
        StaleReplyCode,
        request,
        pluginInstanceEpoch_,
        "ntdel:stale:" + request.commandId);
  }

  return typedRejection(
      DisabledReplyCode,
      request,
      pluginInstanceEpoch_,
      "ntdel:disabled:" + request.commandId);
}
