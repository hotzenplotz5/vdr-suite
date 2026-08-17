#include "suitebridge_native_timer_create.h"

#include <cassert>
#include <cstddef>
#include <stdexcept>
#include <string>

namespace {
std::string fingerprintToken(char digit = 'a')
{
  return "sha256:" + std::string(64, digit);
}

std::string hexToken(const std::string &value)
{
  if (value.empty()) return "-";
  static constexpr char digits[] = "0123456789abcdef";
  std::string encoded;
  encoded.reserve(value.size() * 2);
  for (unsigned char character : value) {
    encoded.push_back(digits[character >> 4U]);
    encoded.push_back(digits[character & 0x0fU]);
  }
  return encoded;
}

std::string request(
    const std::string &commandId,
    const std::string &fingerprint,
    const std::string &operationId,
    const std::string &pluginEpoch = "pie_1",
    const std::string &expectedSpecificationFingerprint = fingerprintToken(),
    const std::string &title = "Tagesschau 20 Uhr")
{
  return
      "EXEC vdr-suite-native/1 vdr.timer.create 1 " + commandId + " " +
      fingerprint + " " + operationId +
      " opr_1 tas_1 ar_1 ir_1 11 ntb_1 " +
      hexToken(expectedSpecificationFingerprint) +
      " job_1 att_1 3 "
      "default agt_1 agi_1 7 100 vdr.timer suitebridge:local suitebridge "
      "9 " + pluginEpoch + " 1 1 vdr.timer.create 101 " +
      hexToken("S19.2E-1-1101-28106") + " " +
      hexToken(title) + " " + hexToken("TV News") + " " +
      hexToken("2026-08-17") + " " + hexToken("---W---") + " " +
      hexToken("2000") + " " + hexToken("2030") + " 50 99 1 0";
}

class CountingCallback final :
    public ISuiteBridgeNativeTimerCreateMutationCallback {
public:
  SuiteBridgeNativeTimerCreateMutationResult result{
      SuiteBridgeNativeTimerCreateMutationDisposition::AppliedUnverified,
      "ntcreate:callback-applied"};
  std::size_t calls = 0;
  bool throwOnCall = false;
  bool reenter = false;
  SuiteBridgeNativeTimerCreateService *service = nullptr;
  std::string reentrantRequest;
  SuiteBridgeCommandResult reentrantReply;
  std::string lastExpectedSpecificationFingerprint;
  std::string lastTitle;

  SuiteBridgeNativeTimerCreateMutationResult CreateTimer(
      const SuiteBridgeNativeTimerCreateRequest &request) override
  {
    ++calls;
    lastExpectedSpecificationFingerprint =
        request.expectedSpecificationFingerprint;
    lastTitle = request.title;
    if (reenter) {
      assert(service != nullptr);
      reentrantReply = service->Handle("NTCREATE", reentrantRequest.c_str());
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
    SuiteBridgeNativeTimerCreateService service("pie_1");
    assert(!service.ExecutionConfigured());

    const SuiteBridgeCommandResult reply = service.Handle("NTCREATE", "CAP 1");
    assert(reply.handled);
    assert(reply.replyCode == 900);
    assert(reply.payload ==
        "vdr-suite-ntcreate-cap/1 vdr.timer.create 1 timer-create disabled "
        "suitebridge pie_1 1 1 disabled");
  }

  {
    SuiteBridgeNativeTimerCreateService service("pie_1");
    const SuiteBridgeCommandResult reply =
        service.Handle("NTCREATE", baseRequest.c_str());
    assert(reply.handled);
    assert(reply.replyCode == 556);
    assert(reply.payload ==
        "vdr-suite-ntcreate-result/1 cmd_1 fp_1 vdr.timer.create 1 pie_1 1 1 "
        "rejected_without_effect disabled ntcreate:disabled:cmd_1");
  }

  {
    SuiteBridgeNativeTimerCreateService service("pie_1");
    const std::string stale = request("cmd_1", "fp_1", "op_1", "pie_0");
    const SuiteBridgeCommandResult reply = service.Handle("NTCREATE", stale.c_str());
    assert(reply.handled);
    assert(reply.replyCode == 555);
    assert(reply.payload ==
        "vdr-suite-ntcreate-result/1 cmd_1 fp_1 vdr.timer.create 1 pie_1 1 1 "
        "rejected_without_effect stale ntcreate:stale:cmd_1");
  }

  {
    SuiteBridgeNativeTimerCreateService service("pie_1");
    const SuiteBridgeCommandResult reply = service.Handle(
        "NTCREATE", "EXEC vdr-suite-native/1 vdr.timer.create 1 broken");
    assert(reply.handled);
    assert(reply.replyCode == 501);
    assert(reply.payload == "vdr-suite-ntcreate-rejected/1 execute-malformed");
  }

  {
    CountingCallback callback;
    SuiteBridgeNativeTimerCreateService service("pie_1", &callback);
    assert(service.ExecutionConfigured());

    const SuiteBridgeCommandResult capability = service.Handle("NTCREATE", "CAP 1");
    assert(capability.handled);
    assert(capability.replyCode == 900);
    assert(capability.payload ==
        "vdr-suite-ntcreate-cap/1 vdr.timer.create 1 timer-create enabled "
        "suitebridge pie_1 1 1 enabled");

    const SuiteBridgeCommandResult first =
        service.Handle("NTCREATE", baseRequest.c_str());
    assert(first.handled);
    assert(first.replyCode == 557);
    assert(first.payload ==
        "vdr-suite-ntcreate-result/1 cmd_1 fp_1 vdr.timer.create 1 pie_1 1 1 "
        "accepted_unverified callback_applied ntcreate:callback-applied");
    assert(callback.calls == 1);
    assert(callback.lastExpectedSpecificationFingerprint == fingerprintToken());
    assert(callback.lastTitle == "Tagesschau 20 Uhr");

    const SuiteBridgeCommandResult replay =
        service.Handle("NTCREATE", baseRequest.c_str());
    assert(replay.replyCode == first.replyCode);
    assert(replay.payload == first.payload);
    assert(callback.calls == 1);

    const std::string conflictingNativeTimerFingerprint =
        request("cmd_1", "fp_1", "op_1", "pie_1", fingerprintToken('b'));
    const SuiteBridgeCommandResult nativeTimerFingerprintConflict =
        service.Handle("NTCREATE", conflictingNativeTimerFingerprint.c_str());
    assert(nativeTimerFingerprintConflict.replyCode == 559);
    assert(nativeTimerFingerprintConflict.payload ==
        "vdr-suite-ntcreate-result/1 cmd_1 fp_1 vdr.timer.create 1 pie_1 1 1 "
        "rejected_without_effect replay_conflict ntcreate:replay-conflict:cmd_1");
    assert(callback.calls == 1);

    const std::string conflictingFingerprint = request("cmd_1", "fp_2", "op_1");
    const SuiteBridgeCommandResult conflict =
        service.Handle("NTCREATE", conflictingFingerprint.c_str());
    assert(conflict.replyCode == 559);
    assert(conflict.payload ==
        "vdr-suite-ntcreate-result/1 cmd_1 fp_2 vdr.timer.create 1 pie_1 1 1 "
        "rejected_without_effect replay_conflict ntcreate:replay-conflict:cmd_1");
    assert(callback.calls == 1);

    const std::string conflictingOperation = request("cmd_1", "fp_3", "op_2");
    const SuiteBridgeCommandResult commandConflict =
        service.Handle("NTCREATE", conflictingOperation.c_str());
    assert(commandConflict.replyCode == 559);
    assert(callback.calls == 1);
  }

  {
    SuiteBridgeNativeTimerCreateService service("pie_1");
    const std::string malformedDigest =
        "sha256:" + std::string(63, 'a') + "g";
    const std::string invalidFingerprint =
        request("cmd_i", "fp_i", "op_i", "pie_1", malformedDigest);
    const SuiteBridgeCommandResult reply =
        service.Handle("NTCREATE", invalidFingerprint.c_str());
    assert(reply.handled);
    assert(reply.replyCode == 501);
    assert(reply.payload == "vdr-suite-ntcreate-rejected/1 execute-malformed");
  }

  {
    CountingCallback callback;
    SuiteBridgeNativeTimerCreateService service("pie_1", &callback);
    const std::string reentrant = request("cmd_r", "fp_r", "op_r");
    callback.reenter = true;
    callback.service = &service;
    callback.reentrantRequest = reentrant;

    const SuiteBridgeCommandResult first =
        service.Handle("NTCREATE", reentrant.c_str());
    assert(callback.calls == 1);
    assert(callback.reentrantReply.handled);
    assert(callback.reentrantReply.replyCode == 558);
    assert(callback.reentrantReply.payload ==
        "vdr-suite-ntcreate-result/1 cmd_r fp_r vdr.timer.create 1 pie_1 1 1 "
        "outcome_unknown in_progress ntcreate:in-progress:cmd_r");
    assert(first.replyCode == 557);

    callback.reenter = false;
    const SuiteBridgeCommandResult replay =
        service.Handle("NTCREATE", reentrant.c_str());
    assert(replay.replyCode == first.replyCode);
    assert(replay.payload == first.payload);
    assert(callback.calls == 1);
  }

  {
    CountingCallback callback;
    callback.result = {
        SuiteBridgeNativeTimerCreateMutationDisposition::OutcomeUnknown,
        "ntcreate:callback-unknown"};
    SuiteBridgeNativeTimerCreateService service("pie_1", &callback);
    const std::string unknownRequest = request("cmd_u", "fp_u", "op_u");

    const SuiteBridgeCommandResult first =
        service.Handle("NTCREATE", unknownRequest.c_str());
    assert(first.replyCode == 558);
    assert(first.payload ==
        "vdr-suite-ntcreate-result/1 cmd_u fp_u vdr.timer.create 1 pie_1 1 1 "
        "outcome_unknown callback_unknown ntcreate:callback-unknown");
    const SuiteBridgeCommandResult replay =
        service.Handle("NTCREATE", unknownRequest.c_str());
    assert(replay.replyCode == first.replyCode);
    assert(replay.payload == first.payload);
    assert(callback.calls == 1);
  }

  {
    CountingCallback callback;
    callback.throwOnCall = true;
    SuiteBridgeNativeTimerCreateService service("pie_1", &callback);
    const std::string throwingRequest = request("cmd_x", "fp_x", "op_x");

    const SuiteBridgeCommandResult first =
        service.Handle("NTCREATE", throwingRequest.c_str());
    assert(first.replyCode == 558);
    assert(first.payload ==
        "vdr-suite-ntcreate-result/1 cmd_x fp_x vdr.timer.create 1 pie_1 1 1 "
        "outcome_unknown callback_unknown ntcreate:callback-exception:cmd_x");
    const SuiteBridgeCommandResult replay =
        service.Handle("NTCREATE", throwingRequest.c_str());
    assert(replay.replyCode == first.replyCode);
    assert(replay.payload == first.payload);
    assert(callback.calls == 1);
  }

  {
    CountingCallback callback;
    SuiteBridgeNativeTimerCreateService service("pie_1", &callback, 1);
    const std::string firstRequest = request("cmd_a", "fp_a", "op_a");
    const std::string secondRequest = request("cmd_b", "fp_b", "op_b");

    const SuiteBridgeCommandResult first =
        service.Handle("NTCREATE", firstRequest.c_str());
    assert(first.replyCode == 557);
    assert(callback.calls == 1);

    const SuiteBridgeCommandResult full =
        service.Handle("NTCREATE", secondRequest.c_str());
    assert(full.replyCode == 560);
    assert(full.payload ==
        "vdr-suite-ntcreate-result/1 cmd_b fp_b vdr.timer.create 1 pie_1 1 1 "
        "rejected_without_effect ledger_full ntcreate:ledger-full:cmd_b");
    assert(callback.calls == 1);
  }

  {
    SuiteBridgeNativeTimerCreateService service("pie_1");
    const SuiteBridgeCommandResult reply = service.Handle("SNAP", nullptr);
    assert(!reply.handled);
  }

  return 0;
}
