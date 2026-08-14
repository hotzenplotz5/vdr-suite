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
constexpr std::size_t NativeFingerprintTokenMaximum = 8192;
constexpr int SuccessReplyCode = 900;
constexpr int MalformedReplyCode = 501;
constexpr int StaleReplyCode = 555;
constexpr int DisabledReplyCode = 556;
constexpr int AcceptedUnverifiedReplyCode = 557;
constexpr int OutcomeUnknownReplyCode = 558;
constexpr int ReplayConflictReplyCode = 559;
constexpr int ReplayLedgerFullReplyCode = 560;

bool safeToken(const std::string &value, std::size_t maximum = 512)
{
  return !value.empty() && value.size() <= maximum &&
      std::all_of(value.begin(), value.end(), [](unsigned char character) {
        return std::isalnum(character) != 0 || character == '-' ||
            character == '_' || character == '.' || character == ':';
      });
}

bool safeFingerprintToken(const std::string &value)
{
  return !value.empty() && value.size() <= NativeFingerprintTokenMaximum &&
      value.size() % 2 == 0 &&
      std::all_of(value.begin(), value.end(), [](unsigned char character) {
        return (character >= '0' && character <= '9') ||
            (character >= 'a' && character <= 'f');
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

bool parseExecute(
    const char *option,
    SuiteBridgeNativeTimerDeleteRequest &request)
{
  const std::vector<std::string> values = split(option);
  if (values.size() != 30 || values[0] != "EXEC" ||
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
  request.expectedNativeTimerFingerprint = values[10];
  request.timerAssignmentId = values[11];
  request.backendNativeTimerId = values[12];
  request.jobId = values[13];
  request.attemptId = values[14];
  request.backendId = values[16];
  request.agentId = values[17];
  request.agentInstanceId = values[18];
  request.authorityDomain = values[21];
  request.providerId = values[22];
  request.providerKind = values[23];
  request.providerInstanceEpoch = values[25];
  request.requiredCapability = values[28];

  return unsignedValue(values[15], request.claimEpoch) &&
      unsignedValue(values[19], request.backendGeneration) &&
      unsignedValue(values[20], request.controlPlaneClaimedAt) &&
      unsignedValue(values[24], request.ownershipGeneration) &&
      unsignedValue(values[26], request.providerGeneration) &&
      unsignedValue(values[27], request.capabilityRevision) &&
      unsignedValue(values[29], request.localStartingPersistedAt) &&
      safeToken(request.commandId, 192) &&
      safeToken(request.requestFingerprint) &&
      safeToken(request.operationId, 192) &&
      safeToken(request.operationRevision, 192) &&
      safeToken(request.nativeTimerBindingId, 192) &&
      safeToken(request.expectedBindingRevision, 192) &&
      safeFingerprintToken(request.expectedNativeTimerFingerprint) &&
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

void appendCanonical(std::ostringstream &output, const std::string &value)
{
  output << value.size() << ':' << value << '|';
}

void appendCanonical(std::ostringstream &output, std::uint64_t value)
{
  output << value << '|';
}

std::string canonicalRequest(const SuiteBridgeNativeTimerDeleteRequest &request)
{
  std::ostringstream canonical;
  appendCanonical(canonical, request.commandId);
  appendCanonical(canonical, request.requestFingerprint);
  appendCanonical(canonical, request.operationId);
  appendCanonical(canonical, request.operationRevision);
  appendCanonical(canonical, request.nativeTimerBindingId);
  appendCanonical(canonical, request.expectedBindingRevision);
  appendCanonical(canonical, request.expectedNativeTimerFingerprint);
  appendCanonical(canonical, request.timerAssignmentId);
  appendCanonical(canonical, request.backendNativeTimerId);
  appendCanonical(canonical, request.jobId);
  appendCanonical(canonical, request.attemptId);
  appendCanonical(canonical, request.claimEpoch);
  appendCanonical(canonical, request.backendId);
  appendCanonical(canonical, request.agentId);
  appendCanonical(canonical, request.agentInstanceId);
  appendCanonical(canonical, request.backendGeneration);
  appendCanonical(canonical, request.controlPlaneClaimedAt);
  appendCanonical(canonical, request.authorityDomain);
  appendCanonical(canonical, request.providerId);
  appendCanonical(canonical, request.providerKind);
  appendCanonical(canonical, request.ownershipGeneration);
  appendCanonical(canonical, request.providerInstanceEpoch);
  appendCanonical(canonical, request.providerGeneration);
  appendCanonical(canonical, request.capabilityRevision);
  appendCanonical(canonical, request.requiredCapability);
  appendCanonical(canonical, request.localStartingPersistedAt);
  return canonical.str();
}

SuiteBridgeCommandResult genericRejection(int replyCode, const char *reason)
{
  SuiteBridgeCommandResult result;
  result.handled = true;
  result.replyCode = replyCode;
  result.payload = std::string("vdr-suite-ntdel-rejected/1 ") + reason;
  return result;
}

SuiteBridgeCommandResult typedResult(
    int replyCode,
    const SuiteBridgeNativeTimerDeleteRequest &request,
    const std::string &pluginInstanceEpoch,
    const char *disposition,
    const char *reason,
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
          << disposition << ' ' << reason << ' ' << evidenceReference;
  result.payload = payload.str();
  return result;
}

SuiteBridgeCommandResult typedRejection(
    int replyCode,
    const SuiteBridgeNativeTimerDeleteRequest &request,
    const std::string &pluginInstanceEpoch,
    const char *reason,
    const std::string &evidenceReference)
{
  return typedResult(
      replyCode,
      request,
      pluginInstanceEpoch,
      "rejected_without_effect",
      reason,
      evidenceReference);
}

SuiteBridgeCommandResult mutationReply(
    const SuiteBridgeNativeTimerDeleteRequest &request,
    const std::string &pluginInstanceEpoch,
    const SuiteBridgeNativeTimerDeleteMutationResult &mutationResult)
{
  switch (mutationResult.disposition) {
    case SuiteBridgeNativeTimerDeleteMutationDisposition::AppliedUnverified:
      return typedResult(
          AcceptedUnverifiedReplyCode,
          request,
          pluginInstanceEpoch,
          "accepted_unverified",
          "callback_applied",
          mutationResult.evidenceReference);
    case SuiteBridgeNativeTimerDeleteMutationDisposition::RejectedWithoutEffect:
      return typedResult(
          DisabledReplyCode,
          request,
          pluginInstanceEpoch,
          "rejected_without_effect",
          "callback_rejected",
          mutationResult.evidenceReference);
    case SuiteBridgeNativeTimerDeleteMutationDisposition::OutcomeUnknown:
      return typedResult(
          OutcomeUnknownReplyCode,
          request,
          pluginInstanceEpoch,
          "outcome_unknown",
          "callback_unknown",
          mutationResult.evidenceReference);
  }
  return typedResult(
      OutcomeUnknownReplyCode,
      request,
      pluginInstanceEpoch,
      "outcome_unknown",
      "callback_invalid",
      "ntdel:callback-invalid:" + request.commandId);
}

SuiteBridgeNativeTimerDeleteMutationResult normalizedCallbackResult(
    const SuiteBridgeNativeTimerDeleteRequest &request,
    SuiteBridgeNativeTimerDeleteMutationResult result)
{
  if (!safeToken(result.evidenceReference)) {
    result.disposition =
        SuiteBridgeNativeTimerDeleteMutationDisposition::OutcomeUnknown;
    result.evidenceReference = "ntdel:callback-invalid:" + request.commandId;
  }
  return result;
}
} // namespace

SuiteBridgeNativeTimerDeleteService::SuiteBridgeNativeTimerDeleteService(
    std::string pluginInstanceEpoch,
    ISuiteBridgeNativeTimerDeleteMutationCallback *mutationCallback,
    std::size_t maximumReplayEntries)
    : pluginInstanceEpoch_(std::move(pluginInstanceEpoch)),
      mutationCallback_(mutationCallback),
      maximumReplayEntries_(maximumReplayEntries)
{
  if (!safeToken(pluginInstanceEpoch_, 192)) pluginInstanceEpoch_ = "pie_invalid";
}

bool SuiteBridgeNativeTimerDeleteService::ExecutionConfigured() const noexcept
{
  return mutationCallback_ != nullptr;
}

SuiteBridgeCommandResult SuiteBridgeNativeTimerDeleteService::Handle(
    const char *command,
    const char *option)
{
  if (command == nullptr || strcasecmp(command, "NTDEL") != 0) return {};
  const std::vector<std::string> values = split(option);
  if (values.empty()) return genericRejection(MalformedReplyCode, "malformed");
  if (values.front() == "CAP") return capability(option);
  if (values.front() == "EXEC") return execute(option);
  return genericRejection(MalformedReplyCode, "unsupported");
}

SuiteBridgeCommandResult SuiteBridgeNativeTimerDeleteService::capability(
    const char *option) const
{
  const std::vector<std::string> values = split(option);
  if (values.size() != 2 || values[0] != "CAP" || values[1] != "1") {
    return genericRejection(MalformedReplyCode, "capability-schema-unsupported");
  }

  const char *state = ExecutionConfigured() ? "enabled" : "disabled";
  SuiteBridgeCommandResult result;
  result.handled = true;
  result.replyCode = SuccessReplyCode;
  std::ostringstream payload;
  payload << CapabilityProtocol << ' ' << NativeOperation << ' '
          << NativeOperationSchema << " timer-delete " << state << ' '
          << ProviderKind << ' ' << pluginInstanceEpoch_ << ' '
          << ProviderGeneration << ' ' << CapabilityRevision << ' ' << state;
  result.payload = payload.str();
  return result;
}

SuiteBridgeCommandResult SuiteBridgeNativeTimerDeleteService::execute(
    const char *option)
{
  SuiteBridgeNativeTimerDeleteRequest request;
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
        "stale",
        "ntdel:stale:" + request.commandId);
  }

  if (!ExecutionConfigured()) {
    return typedRejection(
        DisabledReplyCode,
        request,
        pluginInstanceEpoch_,
        "disabled",
        "ntdel:disabled:" + request.commandId);
  }

  bool replayed = false;
  bool inProgress = false;
  bool conflict = false;
  bool ledgerFull = false;
  const std::string canonical = canonicalRequest(request);
  const SuiteBridgeNativeTimerDeleteMutationResult result = executeReserved(
      request,
      canonical,
      replayed,
      inProgress,
      conflict,
      ledgerFull);

  if (conflict) {
    return typedRejection(
        ReplayConflictReplyCode,
        request,
        pluginInstanceEpoch_,
        "replay_conflict",
        "ntdel:replay-conflict:" + request.commandId);
  }
  if (ledgerFull) {
    return typedRejection(
        ReplayLedgerFullReplyCode,
        request,
        pluginInstanceEpoch_,
        "ledger_full",
        "ntdel:ledger-full:" + request.commandId);
  }
  if (inProgress) {
    return typedResult(
        OutcomeUnknownReplyCode,
        request,
        pluginInstanceEpoch_,
        "outcome_unknown",
        "in_progress",
        "ntdel:in-progress:" + request.commandId);
  }

  (void)replayed;
  return mutationReply(request, pluginInstanceEpoch_, result);
}

SuiteBridgeNativeTimerDeleteMutationResult
SuiteBridgeNativeTimerDeleteService::executeReserved(
    const SuiteBridgeNativeTimerDeleteRequest &request,
    const std::string &canonical,
    bool &replayed,
    bool &inProgress,
    bool &conflict,
    bool &ledgerFull)
{
  replayed = false;
  inProgress = false;
  conflict = false;
  ledgerFull = false;

  {
    std::lock_guard<std::mutex> lock(replayMutex_);

    const auto commandOwner = operationByCommandId_.find(request.commandId);
    if (commandOwner != operationByCommandId_.end() &&
        commandOwner->second != request.operationId) {
      conflict = true;
      return {};
    }

    const auto existing = replayByOperationId_.find(request.operationId);
    if (existing != replayByOperationId_.end()) {
      const ReplayEntry &entry = existing->second;
      if (entry.commandId != request.commandId ||
          entry.requestFingerprint != request.requestFingerprint ||
          entry.canonicalRequest != canonical) {
        conflict = true;
        return {};
      }
      if (!entry.terminal) {
        inProgress = true;
        return {};
      }
      replayed = true;
      return entry.result;
    }

    if (replayByOperationId_.size() >= maximumReplayEntries_) {
      ledgerFull = true;
      return {};
    }

    ReplayEntry entry;
    entry.commandId = request.commandId;
    entry.requestFingerprint = request.requestFingerprint;
    entry.canonicalRequest = canonical;
    replayByOperationId_.emplace(request.operationId, std::move(entry));
    operationByCommandId_.emplace(request.commandId, request.operationId);
  }

  SuiteBridgeNativeTimerDeleteMutationResult callbackResult;
  try {
    callbackResult = normalizedCallbackResult(
        request,
        mutationCallback_->DeleteTimer(request));
  } catch (...) {
    callbackResult.disposition =
        SuiteBridgeNativeTimerDeleteMutationDisposition::OutcomeUnknown;
    callbackResult.evidenceReference =
        "ntdel:callback-exception:" + request.commandId;
  }

  {
    std::lock_guard<std::mutex> lock(replayMutex_);
    const auto existing = replayByOperationId_.find(request.operationId);
    if (existing != replayByOperationId_.end() &&
        existing->second.commandId == request.commandId &&
        existing->second.requestFingerprint == request.requestFingerprint &&
        existing->second.canonicalRequest == canonical) {
      existing->second.result = callbackResult;
      existing->second.terminal = true;
    }
  }

  return callbackResult;
}
