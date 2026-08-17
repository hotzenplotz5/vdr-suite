#ifndef VDR_SUITE_BRIDGE_NATIVE_TIMER_CREATE_H
#define VDR_SUITE_BRIDGE_NATIVE_TIMER_CREATE_H

#include "suitebridge_command_result.h"

#include <cstddef>
#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_map>

struct SuiteBridgeNativeTimerCreateRequest final {
  std::string commandId;
  std::string requestFingerprint;
  std::string operationId;
  std::string operationRevision;
  std::string timerAssignmentId;
  std::string expectedAssignmentRevision;
  std::string expectedIntentRevision;
  std::uint64_t assignmentEpoch = 0;
  std::string nativeTimerBindingId;
  std::string expectedSpecificationFingerprint;
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
  std::string channelId;
  std::string title;
  std::string directory;
  std::string day;
  std::string weekdays;
  std::string startTime;
  std::string endTime;
  std::uint64_t priority = 0;
  std::uint64_t lifetime = 0;
  bool enabled = false;
  bool vps = false;
};

enum class SuiteBridgeNativeTimerCreateMutationDisposition {
  AppliedUnverified,
  RejectedWithoutEffect,
  OutcomeUnknown,
};

struct SuiteBridgeNativeTimerCreateMutationResult final {
  SuiteBridgeNativeTimerCreateMutationDisposition disposition =
      SuiteBridgeNativeTimerCreateMutationDisposition::OutcomeUnknown;
  std::string evidenceReference;
};

class ISuiteBridgeNativeTimerCreateMutationCallback {
public:
  virtual ~ISuiteBridgeNativeTimerCreateMutationCallback() = default;

  virtual SuiteBridgeNativeTimerCreateMutationResult CreateTimer(
      const SuiteBridgeNativeTimerCreateRequest &request) = 0;
};

class SuiteBridgeNativeTimerCreateService final {
public:
  explicit SuiteBridgeNativeTimerCreateService(
      std::string pluginInstanceEpoch,
      ISuiteBridgeNativeTimerCreateMutationCallback *mutationCallback = nullptr,
      std::size_t maximumReplayEntries = 4096);

  SuiteBridgeCommandResult Handle(
      const char *command,
      const char *option);

  bool ExecutionConfigured() const noexcept;

private:
  struct ReplayEntry final {
    std::string commandId;
    std::string requestFingerprint;
    std::string canonicalRequest;
    bool terminal = false;
    SuiteBridgeNativeTimerCreateMutationResult result;
  };

  SuiteBridgeCommandResult capability(const char *option) const;
  SuiteBridgeCommandResult execute(const char *option);
  SuiteBridgeNativeTimerCreateMutationResult executeReserved(
      const SuiteBridgeNativeTimerCreateRequest &request,
      const std::string &canonicalRequest,
      bool &replayed,
      bool &inProgress,
      bool &conflict,
      bool &ledgerFull);

  std::string pluginInstanceEpoch_;
  ISuiteBridgeNativeTimerCreateMutationCallback *mutationCallback_ = nullptr;
  std::size_t maximumReplayEntries_ = 4096;
  std::mutex replayMutex_;
  std::unordered_map<std::string, ReplayEntry> replayByOperationId_;
  std::unordered_map<std::string, std::string> operationByCommandId_;
};

#endif
