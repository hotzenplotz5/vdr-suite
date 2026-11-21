#ifndef VDR_SUITE_BRIDGE_NATIVE_TIMER_MODIFY_H
#define VDR_SUITE_BRIDGE_NATIVE_TIMER_MODIFY_H

#include "suitebridge_command_result.h"

#include <cstddef>
#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_map>

enum class SuiteBridgeNativeTimerModifyKind {
  Update,
  Toggle,
};

struct SuiteBridgeNativeTimerModifyRequest final {
  SuiteBridgeNativeTimerModifyKind kind = SuiteBridgeNativeTimerModifyKind::Update;
  std::string commandId;
  std::string requestFingerprint;
  std::string operationId;
  std::string operationRevision;
  std::string nativeTimerBindingId;
  std::string expectedBindingRevision;
  std::string expectedNativeTimerFingerprint;
  std::string timerAssignmentId;
  std::string backendNativeTimerId;
  std::string channelId;
  std::string title;
  std::string directory;
  std::string day;
  std::string weekdays;
  std::string startTime;
  std::string endTime;
  std::uint32_t priority = 0;
  std::uint32_t lifetime = 0;
  bool enabled = false;
  bool vps = false;
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

enum class SuiteBridgeNativeTimerModifyMutationDisposition {
  AppliedUnverified,
  RejectedWithoutEffect,
  OutcomeUnknown,
};

struct SuiteBridgeNativeTimerModifyMutationResult final {
  SuiteBridgeNativeTimerModifyMutationDisposition disposition =
      SuiteBridgeNativeTimerModifyMutationDisposition::OutcomeUnknown;
  std::string evidenceReference;
};

class ISuiteBridgeNativeTimerModifyMutationCallback {
public:
  virtual ~ISuiteBridgeNativeTimerModifyMutationCallback() = default;
  virtual SuiteBridgeNativeTimerModifyMutationResult ModifyTimer(
      const SuiteBridgeNativeTimerModifyRequest &request) = 0;
};

class SuiteBridgeNativeTimerModifyService final {
public:
  explicit SuiteBridgeNativeTimerModifyService(
      std::string pluginInstanceEpoch,
      ISuiteBridgeNativeTimerModifyMutationCallback *mutationCallback = nullptr,
      std::size_t maximumReplayEntries = 4096);

  SuiteBridgeCommandResult Handle(const char *command, const char *option);
  bool ExecutionConfigured() const noexcept;

private:
  struct ReplayEntry final {
    std::string commandId;
    std::string requestFingerprint;
    std::string canonicalRequest;
    bool terminal = false;
    SuiteBridgeNativeTimerModifyMutationResult result;
  };

  SuiteBridgeCommandResult capability(const char *option) const;
  SuiteBridgeCommandResult execute(const char *option);
  SuiteBridgeNativeTimerModifyMutationResult executeReserved(
      const SuiteBridgeNativeTimerModifyRequest &request,
      const std::string &canonicalRequest,
      bool &inProgress,
      bool &conflict,
      bool &ledgerFull);

  std::string pluginInstanceEpoch_;
  ISuiteBridgeNativeTimerModifyMutationCallback *mutationCallback_ = nullptr;
  std::size_t maximumReplayEntries_ = 4096;
  std::mutex replayMutex_;
  std::unordered_map<std::string, ReplayEntry> replayByOperationId_;
  std::unordered_map<std::string, std::string> operationByCommandId_;
};

#endif
