#include "suitebridge_native_timer_delete.h"

#include <cassert>
#include <cstddef>
#include <stdexcept>
#include <string>

namespace {
std::string request(
    const std::string &commandId,
    const std::string &fingerprint,
    const std::string &operationId,
    const std::string &pluginEpoch = "pie_1",
    const std::string &expectedNativeTimerFingerprint = "ntfp_1")
{
  return
      "EXEC vdr-suite-native/1 vdr.timer.delete 1 " + commandId + " " +
      fingerprint + " " + operationId +
      " opr_1 ntb_1 nbr_1 " + expectedNativeTimerFingerprint +
      " tas_1 42 job_1 att_1 3 "
      "default agt_1 agi_1 7 100 vdr.timer suitebridge:local suitebridge "
      "9 " + pluginEpoch + " 1 1 vdr.timer.delete 101";
}

class CountingCallback final :
    public ISuiteBridgeNativeTimerDeleteMutationCallback {
public:
  SuiteBridgeNativeTimerDeleteMutationResult result{
      SuiteBridgeNativeTimerDeleteMutationDisposition::AppliedUnverified,
      "ntdel:callback-applied"};
  std::size_t calls = 0;
  bool throwOnCall = false;
  bool reenter = false;
  SuiteBridgeNativeTimerDeleteService *service = nullptr;
  std::string reentrantRequest;
  SuiteBridgeCommandResult reentrantReply;
  std::string lastExpectedNativeTimerFingerprint;

  SuiteBridgeNativeTimerDeleteMutationResult DeleteTimer(
      const SuiteBridgeNativeTimerDeleteRequest &request) override
  {
    ++calls;
    lastExpectedNativeTimerFingerprint =
        request.expectedNativeTimerFingerprint;
    if (reenter) {
      assert(service != nullptr);
      reentrantReply = service->Handle("NTDEL", reentrantRequest.c_str());
    }
    if (throwOnCall) throw std::runtime_error("synthetic callback failure");
    return result;
  }
};
} // namespace

int main()
{
  const std::string baseRequest = request("cmd_1", "fp_1", "op_1");

  {
    SuiteBridgeNativeTimerDeleteService service("pie_1");
    assert(!service.ExecutionConfigured());

    const SuiteBridgeCommandResult reply = service.Handle("NTDEL", "CAP 1");
    assert(reply.handled);
    assert(reply.replyCode == 900);
    assert(reply.payload ==
        "vdr-suite-ntdel-cap/1 vdr.timer.delete 1 timer-delete disabled "
        "suitebridge pie_1 1 1 disabled");
  }

  {
    SuiteBridgeNativeTimerDeleteService service("pie_1");
    const SuiteBridgeCommandResult reply =
        service.Handle("NTDEL", baseRequest.c_str());
    assert(reply.handled);
    assert(reply.replyCode == 556);
    assert(reply.payload ==
        "vdr-suite-ntdel-result/1 cmd_1 fp_1 vdr.timer.delete 1 pie_1 1 1 "
        "rejected_without_effect disabled ntdel:disabled:cmd_1");
  }

  {
    SuiteBridgeNativeTimerDeleteService service("pie_1");
    const std::string stale = request("cmd_1", "fp_1", "op_1", "pie_0");
    const SuiteBridgeCommandResult reply = service.Handle("NTDEL", stale.c_str());
    assert(reply.handled);
    assert(reply.replyCode == 555);
    assert(reply.payload ==
        "vdr-suite-ntdel-result/1 cmd_1 fp_1 vdr.timer.delete 1 pie_1 1 1 "
        "rejected_without_effect stale ntdel:stale:cmd_1");
  }

  {
    SuiteBridgeNativeTimerDeleteService service("pie_1");
    const SuiteBridgeCommandResult reply = service.Handle(
        "NTDEL", "EXEC vdr-suite-native/1 vdr.timer.delete 1 broken");
    assert(reply.handled);
    assert(reply.replyCode == 501);
    assert(reply.payload == "vdr-suite-ntdel-rejected/1 execute-malformed");
  }

  {
    CountingCallback callback;
    SuiteBridgeNativeTimerDeleteService service("pie_1", &callback);
    assert(service.ExecutionConfigured());

    const SuiteBridgeCommandResult capability = service.Handle("NTDEL", "CAP 1");
    assert(capability.handled);
    assert(capability.replyCode == 900);
    assert(capability.payload ==
        "vdr-suite-ntdel-cap/1 vdr.timer.delete 1 timer-delete enabled "
        "suitebridge pie_1 1 1 enabled");

    const SuiteBridgeCommandResult first =
        service.Handle("NTDEL", baseRequest.c_str());
    assert(first.handled);
    assert(first.replyCode == 557);
    assert(first.payload ==
        "vdr-suite-ntdel-result/1 cmd_1 fp_1 vdr.timer.delete 1 pie_1 1 1 "
        "accepted_unverified callback_applied ntdel:callback-applied");
    assert(callback.calls == 1);
    assert(callback.lastExpectedNativeTimerFingerprint == "ntfp_1");

    const SuiteBridgeCommandResult replay =
        service.Handle("NTDEL", baseRequest.c_str());
    assert(replay.replyCode == first.replyCode);
    assert(replay.payload == first.payload);
    assert(callback.calls == 1);

    const std::string conflictingNativeTimerFingerprint =
        request("cmd_1", "fp_1", "op_1", "pie_1", "ntfp_2");
    const SuiteBridgeCommandResult nativeTimerFingerprintConflict =
        service.Handle("NTDEL", conflictingNativeTimerFingerprint.c_str());
    assert(nativeTimerFingerprintConflict.replyCode == 559);
    assert(nativeTimerFingerprintConflict.payload ==
        "vdr-suite-ntdel-result/1 cmd_1 fp_1 vdr.timer.delete 1 pie_1 1 1 "
        "rejected_without_effect replay_conflict ntdel:replay-conflict:cmd_1");
    assert(callback.calls == 1);

    const std::string conflictingFingerprint = request("cmd_1", "fp_2", "op_1");
    const SuiteBridgeCommandResult conflict =
        service.Handle("NTDEL", conflictingFingerprint.c_str());
    assert(conflict.replyCode == 559);
    assert(conflict.payload ==
        "vdr-suite-ntdel-result/1 cmd_1 fp_2 vdr.timer.delete 1 pie_1 1 1 "
        "rejected_without_effect replay_conflict ntdel:replay-conflict:cmd_1");
    assert(callback.calls == 1);

    const std::string conflictingOperation = request("cmd_1", "fp_3", "op_2");
    const SuiteBridgeCommandResult commandConflict =
        service.Handle("NTDEL", conflictingOperation.c_str());
    assert(commandConflict.replyCode == 559);
    assert(callback.calls == 1);
  }

  {
    CountingCallback callback;
    SuiteBridgeNativeTimerDeleteService service("pie_1", &callback);
    const std::string reentrant = request("cmd_r", "fp_r", "op_r");
    callback.reenter = true;
    callback.service = &service;
    callback.reentrantRequest = reentrant;

    const SuiteBridgeCommandResult first =
        service.Handle("NTDEL", reentrant.c_str());
    assert(callback.calls == 1);
    assert(callback.reentrantReply.handled);
    assert(callback.reentrantReply.replyCode == 558);
    assert(callback.reentrantReply.payload ==
        "vdr-suite-ntdel-result/1 cmd_r fp_r vdr.timer.delete 1 pie_1 1 1 "
        "outcome_unknown in_progress ntdel:in-progress:cmd_r");
    assert(first.replyCode == 557);

    callback.reenter = false;
    const SuiteBridgeCommandResult replay =
        service.Handle("NTDEL", reentrant.c_str());
    assert(replay.replyCode == first.replyCode);
    assert(replay.payload == first.payload);
    assert(callback.calls == 1);
  }

  {
    CountingCallback callback;
    callback.result = {
        SuiteBridgeNativeTimerDeleteMutationDisposition::OutcomeUnknown,
        "ntdel:callback-unknown"};
    SuiteBridgeNativeTimerDeleteService service("pie_1", &callback);
    const std::string unknownRequest = request("cmd_u", "fp_u", "op_u");

    const SuiteBridgeCommandResult first =
        service.Handle("NTDEL", unknownRequest.c_str());
    assert(first.replyCode == 558);
    assert(first.payload ==
        "vdr-suite-ntdel-result/1 cmd_u fp_u vdr.timer.delete 1 pie_1 1 1 "
        "outcome_unknown callback_unknown ntdel:callback-unknown");
    const SuiteBridgeCommandResult replay =
        service.Handle("NTDEL", unknownRequest.c_str());
    assert(replay.replyCode == first.replyCode);
    assert(replay.payload == first.payload);
    assert(callback.calls == 1);
  }

  {
    CountingCallback callback;
    callback.throwOnCall = true;
    SuiteBridgeNativeTimerDeleteService service("pie_1", &callback);
    const std::string throwingRequest = request("cmd_x", "fp_x", "op_x");

    const SuiteBridgeCommandResult first =
        service.Handle("NTDEL", throwingRequest.c_str());
    assert(first.replyCode == 558);
    assert(first.payload ==
        "vdr-suite-ntdel-result/1 cmd_x fp_x vdr.timer.delete 1 pie_1 1 1 "
        "outcome_unknown callback_unknown ntdel:callback-exception:cmd_x");
    const SuiteBridgeCommandResult replay =
        service.Handle("NTDEL", throwingRequest.c_str());
    assert(replay.replyCode == first.replyCode);
    assert(replay.payload == first.payload);
    assert(callback.calls == 1);
  }

  {
    CountingCallback callback;
    SuiteBridgeNativeTimerDeleteService service("pie_1", &callback, 1);
    const std::string firstRequest = request("cmd_a", "fp_a", "op_a");
    const std::string secondRequest = request("cmd_b", "fp_b", "op_b");

    const SuiteBridgeCommandResult first =
        service.Handle("NTDEL", firstRequest.c_str());
    assert(first.replyCode == 557);
    assert(callback.calls == 1);

    const SuiteBridgeCommandResult full =
        service.Handle("NTDEL", secondRequest.c_str());
    assert(full.replyCode == 560);
    assert(full.payload ==
        "vdr-suite-ntdel-result/1 cmd_b fp_b vdr.timer.delete 1 pie_1 1 1 "
        "rejected_without_effect ledger_full ntdel:ledger-full:cmd_b");
    assert(callback.calls == 1);
  }

  {
    SuiteBridgeNativeTimerDeleteService service("pie_1");
    const SuiteBridgeCommandResult reply = service.Handle("SNAP", nullptr);
    assert(!reply.handled);
  }

  return 0;
}
