#include "suitebridge_native_timer_modify.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <limits>
#include <sstream>
#include <strings.h>
#include <utility>
#include <vector>

namespace {
constexpr const char *UpdateOperation = "vdr.timer.update";
constexpr const char *ToggleOperation = "vdr.timer.toggle";
constexpr const char *AuthorityDomain = "vdr.timer";
constexpr const char *ProviderId = "suitebridge:local";
constexpr const char *ProviderKind = "suitebridge";
constexpr const char *CapabilityProtocol = "vdr-suite-ntmod-cap/1";
constexpr const char *ResultProtocol = "vdr-suite-ntmod-result/1";
constexpr std::uint64_t ProviderGeneration = 1;
constexpr std::uint64_t CapabilityRevision = 1;
constexpr std::size_t FingerprintLength = 71;
constexpr int SuccessReplyCode = 900;
constexpr int MalformedReplyCode = 501;
constexpr int StaleReplyCode = 555;
constexpr int DisabledReplyCode = 556;
constexpr int AcceptedUnverifiedReplyCode = 557;
constexpr int OutcomeUnknownReplyCode = 558;
constexpr int ReplayConflictReplyCode = 559;
constexpr int ReplayLedgerFullReplyCode = 560;

const char *operation(SuiteBridgeNativeTimerModifyKind kind)
{
  return kind == SuiteBridgeNativeTimerModifyKind::Toggle
      ? ToggleOperation : UpdateOperation;
}

bool safeToken(const std::string &value, std::size_t maximum = 512)
{
  return !value.empty() && value.size() <= maximum &&
      std::all_of(value.begin(), value.end(), [](unsigned char character) {
        return std::isalnum(character) != 0 || character == '-' ||
            character == '_' || character == '.' || character == ':';
      });
}

bool safeFingerprint(const std::string &value)
{
  if (value.size() != FingerprintLength ||
      value.compare(0, 7, "sha256:") != 0) return false;
  return std::all_of(value.begin() + 7, value.end(), [](unsigned char c) {
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f');
  });
}

bool unsignedValue(const std::string &value, std::uint64_t &parsed, bool positive = true)
{
  if (value.empty() || value.size() > 19) return false;
  parsed = 0;
  for (unsigned char character : value) {
    if (character < '0' || character > '9') return false;
    const unsigned digit = static_cast<unsigned>(character - '0');
    if (parsed > (std::numeric_limits<std::uint64_t>::max() - digit) / 10U)
      return false;
    parsed = parsed * 10U + digit;
  }
  return !positive || parsed > 0;
}

bool smallValue(const std::string &value, std::uint32_t &parsed)
{
  std::uint64_t candidate = 0;
  if (!unsignedValue(value, candidate, false) || candidate > 99) return false;
  parsed = static_cast<std::uint32_t>(candidate);
  return true;
}

bool booleanValue(const std::string &value, bool &parsed)
{
  if (value == "0") { parsed = false; return true; }
  if (value == "1") { parsed = true; return true; }
  return false;
}

int hexValue(unsigned char character)
{
  if (character >= '0' && character <= '9') return character - '0';
  if (character >= 'a' && character <= 'f') return character - 'a' + 10;
  if (character >= 'A' && character <= 'F') return character - 'A' + 10;
  return -1;
}

bool decodeText(const std::string &encoded, std::string &value, std::size_t maximum)
{
  value.clear();
  if (encoded == "-") return true;
  if (encoded.empty() || (encoded.size() % 2U) != 0 ||
      encoded.size() > maximum * 2U) return false;
  value.reserve(encoded.size() / 2U);
  for (std::size_t offset = 0; offset < encoded.size(); offset += 2U) {
    const int high = hexValue(static_cast<unsigned char>(encoded[offset]));
    const int low = hexValue(static_cast<unsigned char>(encoded[offset + 1U]));
    if (high < 0 || low < 0) return false;
    const char character = static_cast<char>((high << 4) | low);
    if (character == '\0' || character == '\r' || character == '\n')
      return false;
    value.push_back(character);
  }
  return true;
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
    values.push_back(input.substr(position,
        end == std::string::npos ? std::string::npos : end - position));
    if (values.size() > 48) return {};
    if (end == std::string::npos) break;
    position = end + 1;
  }
  return values;
}

bool validTime(const std::string &value)
{
  if (value.size() != 4) return false;
  for (unsigned char c : value) if (!std::isdigit(c)) return false;
  const int hour = (value[0]-'0')*10 + value[1]-'0';
  const int minute = (value[2]-'0')*10 + value[3]-'0';
  return hour <= 23 && minute <= 59;
}

bool validWeekdays(const std::string &value)
{
  if (value.size() != 7) return false;
  for (unsigned char c : value)
    if (c != '-' && std::isalpha(c) == 0) return false;
  return true;
}

bool parseExecute(const char *option, SuiteBridgeNativeTimerModifyRequest &request)
{
  const std::vector<std::string> v = split(option);
  if (v.size() != 41 || v[0] != "EXEC" ||
      v[1] != "vdr-suite-native/1" || v[3] != "1") return false;
  if (v[2] == UpdateOperation) request.kind = SuiteBridgeNativeTimerModifyKind::Update;
  else if (v[2] == ToggleOperation) request.kind = SuiteBridgeNativeTimerModifyKind::Toggle;
  else return false;

  request.commandId=v[4]; request.requestFingerprint=v[5];
  request.operationId=v[6]; request.operationRevision=v[7];
  request.nativeTimerBindingId=v[8]; request.expectedBindingRevision=v[9];
  request.expectedNativeTimerFingerprint=v[10]; request.timerAssignmentId=v[11];
  request.backendNativeTimerId=v[12];
  if (!decodeText(v[13], request.channelId, 192) ||
      !decodeText(v[14], request.title, 1024) ||
      !decodeText(v[15], request.directory, 1024) ||
      !decodeText(v[16], request.day, 192) ||
      !decodeText(v[17], request.weekdays, 16) ||
      !decodeText(v[18], request.startTime, 8) ||
      !decodeText(v[19], request.endTime, 8)) return false;
  request.jobId=v[24]; request.attemptId=v[25];
  request.backendId=v[27]; request.agentId=v[28]; request.agentInstanceId=v[29];
  request.authorityDomain=v[32]; request.providerId=v[33]; request.providerKind=v[34];
  request.providerInstanceEpoch=v[36]; request.requiredCapability=v[39];

  return smallValue(v[20], request.priority) &&
      smallValue(v[21], request.lifetime) &&
      booleanValue(v[22], request.enabled) && booleanValue(v[23], request.vps) &&
      unsignedValue(v[26], request.claimEpoch) &&
      unsignedValue(v[30], request.backendGeneration) &&
      unsignedValue(v[31], request.controlPlaneClaimedAt) &&
      unsignedValue(v[35], request.ownershipGeneration) &&
      unsignedValue(v[37], request.providerGeneration) &&
      unsignedValue(v[38], request.capabilityRevision) &&
      unsignedValue(v[40], request.localStartingPersistedAt) &&
      safeToken(request.commandId,192) && safeToken(request.requestFingerprint) &&
      safeToken(request.operationId,192) && safeToken(request.operationRevision,192) &&
      safeToken(request.nativeTimerBindingId,192) &&
      safeToken(request.expectedBindingRevision,192) &&
      safeFingerprint(request.expectedNativeTimerFingerprint) &&
      safeToken(request.timerAssignmentId,192) &&
      safeToken(request.backendNativeTimerId,192) &&
      !request.channelId.empty() && validWeekdays(request.weekdays) &&
      validTime(request.startTime) && validTime(request.endTime) &&
      safeToken(request.jobId,192) && safeToken(request.attemptId,192) &&
      safeToken(request.backendId,192) && safeToken(request.agentId,192) &&
      safeToken(request.agentInstanceId,192) && safeToken(request.authorityDomain,192) &&
      safeToken(request.providerId,192) && safeToken(request.providerKind,192) &&
      safeToken(request.providerInstanceEpoch,192) &&
      safeToken(request.requiredCapability,192);
}

void append(std::ostringstream &out, const std::string &value)
{
  out << value.size() << ':' << value << '|';
}
void append(std::ostringstream &out, std::uint64_t value) { out << value << '|'; }
std::string canonicalRequest(const SuiteBridgeNativeTimerModifyRequest &r)
{
  std::ostringstream c;
  append(c, operation(r.kind)); append(c,r.commandId); append(c,r.requestFingerprint);
  append(c,r.operationId); append(c,r.operationRevision); append(c,r.nativeTimerBindingId);
  append(c,r.expectedBindingRevision); append(c,r.expectedNativeTimerFingerprint);
  append(c,r.timerAssignmentId); append(c,r.backendNativeTimerId); append(c,r.channelId);
  append(c,r.title); append(c,r.directory); append(c,r.day); append(c,r.weekdays);
  append(c,r.startTime); append(c,r.endTime); append(c,r.priority); append(c,r.lifetime);
  append(c,r.enabled ? 1U : 0U); append(c,r.vps ? 1U : 0U); append(c,r.jobId);
  append(c,r.attemptId); append(c,r.claimEpoch); append(c,r.backendId); append(c,r.agentId);
  append(c,r.agentInstanceId); append(c,r.backendGeneration); append(c,r.controlPlaneClaimedAt);
  append(c,r.authorityDomain); append(c,r.providerId); append(c,r.providerKind);
  append(c,r.ownershipGeneration); append(c,r.providerInstanceEpoch);
  append(c,r.providerGeneration); append(c,r.capabilityRevision);
  append(c,r.requiredCapability); append(c,r.localStartingPersistedAt);
  return c.str();
}

SuiteBridgeCommandResult genericRejection(int code, const char *reason)
{
  return {true, code, std::string("vdr-suite-ntmod-rejected/1 ") + reason};
}

SuiteBridgeCommandResult typedResult(
    int code, const SuiteBridgeNativeTimerModifyRequest &r,
    const std::string &epoch, const char *disposition,
    const char *reason, const std::string &evidence)
{
  SuiteBridgeCommandResult result;
  result.handled=true; result.replyCode=code;
  std::ostringstream payload;
  payload << ResultProtocol << ' ' << r.commandId << ' ' << r.requestFingerprint
          << ' ' << operation(r.kind) << " 1 " << epoch << ' '
          << ProviderGeneration << ' ' << CapabilityRevision << ' '
          << disposition << ' ' << reason << ' ' << evidence;
  result.payload=payload.str();
  return result;
}

SuiteBridgeCommandResult typedRejection(
    int code, const SuiteBridgeNativeTimerModifyRequest &r,
    const std::string &epoch, const char *reason, const std::string &evidence)
{
  return typedResult(code,r,epoch,"rejected_without_effect",reason,evidence);
}

SuiteBridgeCommandResult mutationReply(
    const SuiteBridgeNativeTimerModifyRequest &r, const std::string &epoch,
    const SuiteBridgeNativeTimerModifyMutationResult &result)
{
  switch (result.disposition) {
    case SuiteBridgeNativeTimerModifyMutationDisposition::AppliedUnverified:
      return typedResult(AcceptedUnverifiedReplyCode,r,epoch,
          "accepted_unverified","callback_applied",result.evidenceReference);
    case SuiteBridgeNativeTimerModifyMutationDisposition::RejectedWithoutEffect:
      return typedResult(DisabledReplyCode,r,epoch,
          "rejected_without_effect","callback_rejected",result.evidenceReference);
    case SuiteBridgeNativeTimerModifyMutationDisposition::OutcomeUnknown:
      return typedResult(OutcomeUnknownReplyCode,r,epoch,
          "outcome_unknown","callback_unknown",result.evidenceReference);
  }
  return typedResult(OutcomeUnknownReplyCode,r,epoch,
      "outcome_unknown","callback_invalid","ntmod:callback-invalid:"+r.commandId);
}

SuiteBridgeNativeTimerModifyMutationResult normalized(
    const SuiteBridgeNativeTimerModifyRequest &r,
    SuiteBridgeNativeTimerModifyMutationResult result)
{
  if (!safeToken(result.evidenceReference)) {
    result.disposition=SuiteBridgeNativeTimerModifyMutationDisposition::OutcomeUnknown;
    result.evidenceReference="ntmod:callback-invalid:"+r.commandId;
  }
  return result;
}
} // namespace

SuiteBridgeNativeTimerModifyService::SuiteBridgeNativeTimerModifyService(
    std::string pluginInstanceEpoch,
    ISuiteBridgeNativeTimerModifyMutationCallback *callback,
    std::size_t maximumReplayEntries)
    : pluginInstanceEpoch_(std::move(pluginInstanceEpoch)),
      mutationCallback_(callback), maximumReplayEntries_(maximumReplayEntries)
{
  if (!safeToken(pluginInstanceEpoch_,192)) pluginInstanceEpoch_="pie_invalid";
}

bool SuiteBridgeNativeTimerModifyService::ExecutionConfigured() const noexcept
{
  return mutationCallback_ != nullptr;
}

SuiteBridgeCommandResult SuiteBridgeNativeTimerModifyService::Handle(
    const char *command, const char *option)
{
  if (command == nullptr || strcasecmp(command,"NTMOD") != 0) return {};
  const auto values=split(option);
  if (values.empty()) return genericRejection(MalformedReplyCode,"malformed");
  if (values.front()=="CAP") return capability(option);
  if (values.front()=="EXEC") return execute(option);
  return genericRejection(MalformedReplyCode,"unsupported");
}

SuiteBridgeCommandResult SuiteBridgeNativeTimerModifyService::capability(
    const char *option) const
{
  const auto v=split(option);
  if (v.size()!=3 || v[0]!="CAP" || v[1]!="1" ||
      (v[2]!="update" && v[2]!="toggle"))
    return genericRejection(MalformedReplyCode,"capability-schema-unsupported");
  const char *op=v[2]=="toggle" ? ToggleOperation : UpdateOperation;
  const char *state=ExecutionConfigured() ? "enabled" : "disabled";
  SuiteBridgeCommandResult result;
  result.handled=true; result.replyCode=SuccessReplyCode;
  std::ostringstream payload;
  payload << CapabilityProtocol << ' ' << op << " 1 timer-modify " << state
          << ' ' << ProviderKind << ' ' << pluginInstanceEpoch_ << ' '
          << ProviderGeneration << ' ' << CapabilityRevision << ' ' << state;
  result.payload=payload.str();
  return result;
}

SuiteBridgeCommandResult SuiteBridgeNativeTimerModifyService::execute(
    const char *option)
{
  SuiteBridgeNativeTimerModifyRequest r;
  if (!parseExecute(option,r))
    return genericRejection(MalformedReplyCode,"execute-malformed");
  const bool currentFence=r.authorityDomain==AuthorityDomain &&
      r.providerId==ProviderId && r.providerKind==ProviderKind &&
      r.providerInstanceEpoch==pluginInstanceEpoch_ &&
      r.providerGeneration==ProviderGeneration &&
      r.capabilityRevision==CapabilityRevision &&
      r.requiredCapability==operation(r.kind) &&
      r.localStartingPersistedAt>=r.controlPlaneClaimedAt;
  if (!currentFence)
    return typedRejection(StaleReplyCode,r,pluginInstanceEpoch_,"stale",
        "ntmod:stale:"+r.commandId);
  if (!ExecutionConfigured())
    return typedRejection(DisabledReplyCode,r,pluginInstanceEpoch_,"disabled",
        "ntmod:disabled:"+r.commandId);

  bool inProgress=false, conflict=false, ledgerFull=false;
  const auto result=executeReserved(r,canonicalRequest(r),inProgress,conflict,ledgerFull);
  if (conflict)
    return typedRejection(ReplayConflictReplyCode,r,pluginInstanceEpoch_,"replay_conflict",
        "ntmod:replay-conflict:"+r.commandId);
  if (ledgerFull)
    return typedRejection(ReplayLedgerFullReplyCode,r,pluginInstanceEpoch_,"ledger_full",
        "ntmod:ledger-full:"+r.commandId);
  if (inProgress)
    return typedResult(OutcomeUnknownReplyCode,r,pluginInstanceEpoch_,
        "outcome_unknown","in_progress","ntmod:in-progress:"+r.commandId);
  return mutationReply(r,pluginInstanceEpoch_,result);
}

SuiteBridgeNativeTimerModifyMutationResult
SuiteBridgeNativeTimerModifyService::executeReserved(
    const SuiteBridgeNativeTimerModifyRequest &r, const std::string &canonical,
    bool &inProgress, bool &conflict, bool &ledgerFull)
{
  inProgress=false; conflict=false; ledgerFull=false;
  {
    std::lock_guard<std::mutex> lock(replayMutex_);
    const auto owner=operationByCommandId_.find(r.commandId);
    if (owner!=operationByCommandId_.end() && owner->second!=r.operationId) {
      conflict=true; return {};
    }
    const auto existing=replayByOperationId_.find(r.operationId);
    if (existing!=replayByOperationId_.end()) {
      const ReplayEntry &entry=existing->second;
      if (entry.commandId!=r.commandId ||
          entry.requestFingerprint!=r.requestFingerprint ||
          entry.canonicalRequest!=canonical) { conflict=true; return {}; }
      if (!entry.terminal) { inProgress=true; return {}; }
      return entry.result;
    }
    if (replayByOperationId_.size()>=maximumReplayEntries_) {
      ledgerFull=true; return {};
    }
    ReplayEntry entry;
    entry.commandId=r.commandId; entry.requestFingerprint=r.requestFingerprint;
    entry.canonicalRequest=canonical;
    replayByOperationId_.emplace(r.operationId,std::move(entry));
    operationByCommandId_.emplace(r.commandId,r.operationId);
  }
  SuiteBridgeNativeTimerModifyMutationResult callbackResult;
  try { callbackResult=normalized(r,mutationCallback_->ModifyTimer(r)); }
  catch (...) {
    callbackResult.disposition=SuiteBridgeNativeTimerModifyMutationDisposition::OutcomeUnknown;
    callbackResult.evidenceReference="ntmod:callback-exception:"+r.commandId;
  }
  {
    std::lock_guard<std::mutex> lock(replayMutex_);
    const auto existing=replayByOperationId_.find(r.operationId);
    if (existing!=replayByOperationId_.end()) {
      existing->second.result=callbackResult;
      existing->second.terminal=true;
    }
  }
  return callbackResult;
}
