#include "suitebridge_recording_cut.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <limits>
#include <sstream>
#include <strings.h>
#include <utility>
#include <vector>

namespace {
constexpr const char *Operation = "vdr.recording.cut";
constexpr const char *AuthorityDomain = "vdr.recording.cut";
constexpr const char *ProviderId = "suitebridge:recording-cut";
constexpr const char *ProviderKind = "suitebridge";
constexpr const char *CapabilityProtocol = "vdr-suite-ncut-cap/1";
constexpr const char *ResultProtocol = "vdr-suite-ncut-result/1";
constexpr std::uint64_t ProviderGeneration = 1;
constexpr std::uint64_t CapabilityRevision = 1;
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

bool lowerHex(const std::string &value, std::size_t length)
{
  return value.size() == length &&
      std::all_of(value.begin(), value.end(), [](unsigned char character) {
        return (character >= '0' && character <= '9') ||
            (character >= 'a' && character <= 'f');
      });
}

bool safeFingerprint(const std::string &value)
{
  return value.size() == 20 &&
      value.compare(0, 4, "fp1_") == 0 &&
      lowerHex(value.substr(4), 16);
}

bool unsignedValue(
    const std::string &value,
    std::uint64_t &parsed,
    bool positive = true)
{
  if (value.empty() || value.size() > 19) return false;
  parsed = 0;
  for (const unsigned char character : value) {
    if (character < '0' || character > '9') return false;
    const unsigned digit = static_cast<unsigned>(character - '0');
    if (parsed > (std::numeric_limits<std::uint64_t>::max() - digit) / 10U)
      return false;
    parsed = parsed * 10U + digit;
  }
  return !positive || parsed > 0;
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
        position,
        end == std::string::npos ? std::string::npos : end - position));
    if (values.size() > 32) return {};
    if (end == std::string::npos) break;
    position = end + 1;
  }
  return values;
}

bool parseExecute(
    const char *option,
    SuiteBridgeRecordingCutRequest &request)
{
  const std::vector<std::string> values = split(option);
  if (values.size() != 27 || values[0] != "EXEC" ||
      values[1] != "vdr-suite-native/1" || values[2] != Operation ||
      values[3] != "1") {
    return false;
  }

  request.commandId = values[4];
  request.requestFingerprint = values[5];
  request.operationId = values[6];
  request.operationRevision = values[7];
  request.recordingKey = values[8];
  request.expectedMarksRevision = values[9];
  request.jobId = values[10];
  request.attemptId = values[11];
  request.backendId = values[13];
  request.agentId = values[14];
  request.agentInstanceId = values[15];
  request.authorityDomain = values[18];
  request.providerId = values[19];
  request.providerKind = values[20];
  request.providerInstanceEpoch = values[22];
  request.requiredCapability = values[25];

  return unsignedValue(values[12], request.claimEpoch) &&
      unsignedValue(values[16], request.backendGeneration) &&
      unsignedValue(values[17], request.controlPlaneClaimedAt) &&
      unsignedValue(values[21], request.ownershipGeneration) &&
      unsignedValue(values[23], request.providerGeneration) &&
      unsignedValue(values[24], request.capabilityRevision) &&
      unsignedValue(values[26], request.localStartingPersistedAt) &&
      safeToken(request.commandId, 192) &&
      safeFingerprint(request.requestFingerprint) &&
      safeToken(request.operationId, 192) &&
      safeToken(request.operationRevision, 192) &&
      lowerHex(request.recordingKey, 32) &&
      lowerHex(request.expectedMarksRevision, 32) &&
      safeToken(request.jobId, 192) &&
      safeToken(request.attemptId, 192) &&
      safeToken(request.backendId, 192) &&
      safeToken(request.agentId, 192) &&
      safeToken(request.agentInstanceId, 192) &&
      safeToken(request.authorityDomain, 192) &&
      safeToken(request.providerId, 192) &&
      safeToken(request.providerKind, 192) &&
      safeToken(request.providerInstanceEpoch, 192) &&
      safeToken(request.requiredCapability, 192);
}

void append(std::ostringstream &out, const std::string &value)
{
  out << value.size() << ':' << value << '|';
}

void append(std::ostringstream &out, std::uint64_t value)
{
  out << value << '|';
}

std::string canonicalRequest(const SuiteBridgeRecordingCutRequest &request)
{
  std::ostringstream canonical;
  append(canonical, Operation);
  append(canonical, request.commandId);
  append(canonical, request.requestFingerprint);
  append(canonical, request.operationId);
  append(canonical, request.operationRevision);
  append(canonical, request.recordingKey);
  append(canonical, request.expectedMarksRevision);
  append(canonical, request.jobId);
  append(canonical, request.attemptId);
  append(canonical, request.claimEpoch);
  append(canonical, request.backendId);
  append(canonical, request.agentId);
  append(canonical, request.agentInstanceId);
  append(canonical, request.backendGeneration);
  append(canonical, request.controlPlaneClaimedAt);
  append(canonical, request.authorityDomain);
  append(canonical, request.providerId);
  append(canonical, request.providerKind);
  append(canonical, request.ownershipGeneration);
  append(canonical, request.providerInstanceEpoch);
  append(canonical, request.providerGeneration);
  append(canonical, request.capabilityRevision);
  append(canonical, request.requiredCapability);
  append(canonical, request.localStartingPersistedAt);
  return canonical.str();
}

SuiteBridgeCommandResult genericRejection(int code, const char *reason)
{
  return {true, code, std::string("vdr-suite-ncut-rejected/1 ") + reason};
}

SuiteBridgeCommandResult typedResult(
    int code,
    const SuiteBridgeRecordingCutRequest &request,
    const std::string &epoch,
    const char *disposition,
    const char *reason,
    const std::string &evidence)
{
  SuiteBridgeCommandResult result;
  result.handled = true;
  result.replyCode = code;
  std::ostringstream payload;
  payload << ResultProtocol << ' ' << request.commandId << ' '
          << request.requestFingerprint << ' ' << Operation << " 1 " << epoch << ' '
          << ProviderGeneration << ' ' << CapabilityRevision << ' '
          << disposition << ' ' << reason << ' ' << evidence;
  result.payload = payload.str();
  return result;
}

SuiteBridgeCommandResult typedRejection(
    int code,
    const SuiteBridgeRecordingCutRequest &request,
    const std::string &epoch,
    const char *reason,
    const std::string &evidence)
{
  return typedResult(
      code,
      request,
      epoch,
      "rejected_without_effect",
      reason,
      evidence);
}

SuiteBridgeCommandResult mutationReply(
    const SuiteBridgeRecordingCutRequest &request,
    const std::string &epoch,
    const SuiteBridgeRecordingCutMutationResult &result)
{
  switch (result.disposition) {
  case SuiteBridgeRecordingCutDisposition::AcceptedUnverified:
    return typedResult(
        AcceptedUnverifiedReplyCode,
        request,
        epoch,
        "accepted_unverified",
        "callback_applied",
        result.evidenceReference);
  case SuiteBridgeRecordingCutDisposition::RejectedWithoutEffect:
    return typedResult(
        DisabledReplyCode,
        request,
        epoch,
        "rejected_without_effect",
        "callback_rejected",
        result.evidenceReference);
  case SuiteBridgeRecordingCutDisposition::OutcomeUnknown:
    return typedResult(
        OutcomeUnknownReplyCode,
        request,
        epoch,
        "outcome_unknown",
        "callback_unknown",
        result.evidenceReference);
  }
  return typedResult(
      OutcomeUnknownReplyCode,
      request,
      epoch,
      "outcome_unknown",
      "callback_invalid",
      "ncut:callback-invalid:" + request.commandId);
}

SuiteBridgeRecordingCutMutationResult normalized(
    const SuiteBridgeRecordingCutRequest &request,
    SuiteBridgeRecordingCutMutationResult result)
{
  if (!safeToken(result.evidenceReference)) {
    result.disposition = SuiteBridgeRecordingCutDisposition::OutcomeUnknown;
    result.evidenceReference = "ncut:callback-invalid:" + request.commandId;
  }
  return result;
}
} // namespace

SuiteBridgeRecordingCutService::SuiteBridgeRecordingCutService(
    std::string pluginInstanceEpoch,
    ISuiteBridgeRecordingCutMutationCallback *mutationCallback,
    std::size_t maximumReplayEntries)
    : pluginInstanceEpoch_(std::move(pluginInstanceEpoch)),
      mutationCallback_(mutationCallback),
      maximumReplayEntries_(maximumReplayEntries)
{
  if (!safeToken(pluginInstanceEpoch_, 192)) pluginInstanceEpoch_ = "pie_invalid";
}

bool SuiteBridgeRecordingCutService::ExecutionConfigured() const noexcept
{
  return mutationCallback_ != nullptr;
}

SuiteBridgeCommandResult SuiteBridgeRecordingCutService::Handle(
    const char *command,
    const char *option)
{
  if (command == nullptr || strcasecmp(command, "NCUT") != 0) return {};
  const std::vector<std::string> values = split(option);
  if (values.empty()) return genericRejection(MalformedReplyCode, "malformed");
  if (values.front() == "CAP") return capability(option);
  if (values.front() == "EXEC") return execute(option);
  return genericRejection(MalformedReplyCode, "unsupported");
}

SuiteBridgeCommandResult SuiteBridgeRecordingCutService::capability(
    const char *option) const
{
  const std::vector<std::string> values = split(option);
  if (values.size() != 3 || values[0] != "CAP" || values[1] != "1" ||
      values[2] != "start") {
    return genericRejection(MalformedReplyCode, "capability-schema-unsupported");
  }
  const char *state = ExecutionConfigured() ? "enabled" : "disabled";
  SuiteBridgeCommandResult result;
  result.handled = true;
  result.replyCode = SuccessReplyCode;
  std::ostringstream payload;
  payload << CapabilityProtocol << ' ' << Operation
          << " 1 recording-cut " << state << ' ' << ProviderKind << ' '
          << pluginInstanceEpoch_ << ' ' << ProviderGeneration << ' '
          << CapabilityRevision << ' ' << state;
  result.payload = payload.str();
  return result;
}

SuiteBridgeCommandResult SuiteBridgeRecordingCutService::execute(
    const char *option)
{
  SuiteBridgeRecordingCutRequest request;
  if (!parseExecute(option, request))
    return genericRejection(MalformedReplyCode, "execute-malformed");

  const bool currentFence = request.authorityDomain == AuthorityDomain &&
      request.providerId == ProviderId && request.providerKind == ProviderKind &&
      request.providerInstanceEpoch == pluginInstanceEpoch_ &&
      request.providerGeneration == ProviderGeneration &&
      request.capabilityRevision == CapabilityRevision &&
      request.requiredCapability == Operation &&
      request.localStartingPersistedAt >= request.controlPlaneClaimedAt;
  if (!currentFence) {
    return typedRejection(
        StaleReplyCode,
        request,
        pluginInstanceEpoch_,
        "stale",
        "ncut:stale:" + request.commandId);
  }
  if (!ExecutionConfigured()) {
    return typedRejection(
        DisabledReplyCode,
        request,
        pluginInstanceEpoch_,
        "disabled",
        "ncut:disabled:" + request.commandId);
  }

  bool inProgress = false;
  bool conflict = false;
  bool ledgerFull = false;
  const SuiteBridgeRecordingCutMutationResult result = executeReserved(
      request,
      canonicalRequest(request),
      inProgress,
      conflict,
      ledgerFull);
  if (conflict) {
    return typedRejection(
        ReplayConflictReplyCode,
        request,
        pluginInstanceEpoch_,
        "replay_conflict",
        "ncut:replay-conflict:" + request.commandId);
  }
  if (ledgerFull) {
    return typedRejection(
        ReplayLedgerFullReplyCode,
        request,
        pluginInstanceEpoch_,
        "ledger_full",
        "ncut:ledger-full:" + request.commandId);
  }
  if (inProgress) {
    return typedResult(
        OutcomeUnknownReplyCode,
        request,
        pluginInstanceEpoch_,
        "outcome_unknown",
        "in_progress",
        "ncut:in-progress:" + request.commandId);
  }
  return mutationReply(request, pluginInstanceEpoch_, result);
}

SuiteBridgeRecordingCutMutationResult
SuiteBridgeRecordingCutService::executeReserved(
    const SuiteBridgeRecordingCutRequest &request,
    const std::string &canonical,
    bool &inProgress,
    bool &conflict,
    bool &ledgerFull)
{
  inProgress = false;
  conflict = false;
  ledgerFull = false;
  {
    std::lock_guard<std::mutex> lock(replayMutex_);
    const auto owner = operationByCommandId_.find(request.commandId);
    if (owner != operationByCommandId_.end() && owner->second != request.operationId) {
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

  SuiteBridgeRecordingCutMutationResult callbackResult;
  try {
    callbackResult = normalized(request, mutationCallback_->StartCut(request));
  } catch (...) {
    callbackResult.disposition = SuiteBridgeRecordingCutDisposition::OutcomeUnknown;
    callbackResult.evidenceReference =
        "ncut:callback-exception:" + request.commandId;
  }
  {
    std::lock_guard<std::mutex> lock(replayMutex_);
    const auto existing = replayByOperationId_.find(request.operationId);
    if (existing != replayByOperationId_.end()) {
      existing->second.result = callbackResult;
      existing->second.terminal = true;
    }
  }
  return callbackResult;
}
