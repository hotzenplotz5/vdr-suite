#include "suitebridge_native_probe.h"

#include <cassert>
#include <string>

namespace {
std::string exec(
    const std::string &commandId,
    const std::string &fingerprint,
    const std::string &epoch,
    const std::string &nonce = "nonce_1")
{
  return "EXEC vdr-suite-native/1 vdr.native.probe 1 " + commandId + " " + fingerprint +
      " op_1 job_1 att_1 1 backend_1 agent_1 agi_1 7 " + epoch + " 1 " + nonce;
}
}

int main()
{
  SuiteBridgeNativeProbeService service("pie_epoch1");

  auto capability = service.Handle("NCAP", "1", [] { return true; });
  assert(capability.handled && capability.replyCode == 900);
  assert(capability.payload.find("\"nativeOperation\":\"vdr.native.probe\"") != std::string::npos);
  assert(capability.payload.find("\"mutations\":\"disabled\"") != std::string::npos);
  assert(capability.payload.find("\"pluginInstanceEpoch\":\"pie_epoch1\"") != std::string::npos);

  const std::string request = exec("cmd_1", "fp_1", "pie_epoch1");
  auto first = service.Handle("NPROBE", request.c_str(), [] { return true; });
  assert(first.handled && first.replyCode == 900);
  assert(first.payload.find("\"receiptCategory\":\"accepted\"") != std::string::npos);
  assert(first.payload.find("\"nativeExecutionSequence\":1") != std::string::npos);
  assert(first.payload.find("\"sideEffectObserved\":false") != std::string::npos);

  auto duplicate = service.Handle("NPROBE", request.c_str(), [] { return true; });
  assert(duplicate.replyCode == 900);
  assert(duplicate.payload.find("\"receiptCategory\":\"duplicate\"") != std::string::npos);
  assert(duplicate.payload.find("\"nativeExecutionSequence\":1") != std::string::npos);

  const std::string conflict = exec("cmd_1", "fp_other", "pie_epoch1");
  auto conflicting = service.Handle("NPROBE", conflict.c_str(), [] { return true; });
  assert(conflicting.replyCode == 554);
  assert(conflicting.payload.find("native_probe_conflict") != std::string::npos);

  auto readback = service.Handle(
      "NPROBE", "READ 1 cmd_1 fp_1 pie_epoch1 1", [] { return true; });
  assert(readback.replyCode == 900);
  assert(readback.payload.find("\"readbackCategory\":\"verified\"") != std::string::npos);
  assert(readback.payload.find("\"duplicateDisposition\":\"exact_replay\"") != std::string::npos);

  auto stale = service.Handle(
      "NPROBE", exec("cmd_2", "fp_2", "pie_epoch2").c_str(), [] { return true; });
  assert(stale.replyCode == 555);
  assert(stale.payload.find("plugin_instance_epoch_stale") != std::string::npos);

  SuiteBridgeNativeProbeService restarted("pie_epoch2");
  auto oldReadback = restarted.Handle(
      "NPROBE", "READ 1 cmd_1 fp_1 pie_epoch1 1", [] { return true; });
  assert(oldReadback.replyCode == 555);

  for (int index = 0; index < 63; ++index) {
    const std::string id = "cmd_fill_" + std::to_string(index);
    const std::string fp = "fp_fill_" + std::to_string(index);
    auto result = service.Handle(
        "NPROBE", exec(id, fp, "pie_epoch1").c_str(), [] { return true; });
    assert(result.replyCode == 900);
  }
  auto full = service.Handle(
      "NPROBE", exec("cmd_overflow", "fp_overflow", "pie_epoch1").c_str(),
      [] { return true; });
  assert(full.replyCode == 451);
  assert(full.payload.find("native_receipt_capacity_exhausted") != std::string::npos);
  auto retained = service.Handle(
      "NPROBE", "READ 1 cmd_1 fp_1 pie_epoch1 1", [] { return true; });
  assert(retained.replyCode == 900);

  return 0;
}
