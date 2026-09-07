#include "../suitebridge_recording_cut.h"

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
struct Callback final : ISuiteBridgeRecordingCutMutationCallback {
  int calls = 0;
  SuiteBridgeRecordingCutRequest last;

  SuiteBridgeRecordingCutMutationResult StartCut(
      const SuiteBridgeRecordingCutRequest &request) override
  {
    ++calls;
    last = request;
    return {
        SuiteBridgeRecordingCutDisposition::AcceptedUnverified,
        "ncut:test:queued"};
  }
};

struct ThrowingCallback final : ISuiteBridgeRecordingCutMutationCallback {
  SuiteBridgeRecordingCutMutationResult StartCut(
      const SuiteBridgeRecordingCutRequest &) override
  {
    throw std::runtime_error("expected test exception");
  }
};

void require(bool value, const char *message)
{
  if (!value) {
    std::cerr << message << '\n';
    std::exit(1);
  }
}

std::string execute(
    const std::string &commandId = "cmd_1",
    const std::string &operationId = "op_1",
    const std::string &operationRevision = "1",
    const std::string &marksRevision =
        "fedcba9876543210fedcba9876543210",
    const std::string &providerEpoch = "pie_1")
{
  return std::string("EXEC vdr-suite-native/1 vdr.recording.cut 1 ") +
      commandId +
      " fp1_aaaaaaaaaaaaaaaa " +
      operationId + " " + operationRevision +
      " 0123456789abcdef0123456789abcdef " + marksRevision +
      " job_1 attempt_1 1 backend_1 agent_1 instance_1 1 100"
      " vdr.recording.cut suitebridge:recording-cut suitebridge 1 " +
      providerEpoch +
      " 1 1 vdr.recording.cut 101";
}
} // namespace

int main()
{
  Callback callback;
  SuiteBridgeRecordingCutService service("pie_1", &callback);

  auto capability = service.Handle("NCUT", "CAP 1 start");
  require(
      capability.handled && capability.replyCode == 900 &&
          capability.payload.find(
              "vdr.recording.cut 1 recording-cut enabled") !=
              std::string::npos,
      "enabled cut capability missing");

  SuiteBridgeRecordingCutService disabled("pie_1");
  capability = disabled.Handle("NCUT", "CAP 1 start");
  require(
      capability.replyCode == 900 &&
          capability.payload.find("recording-cut disabled") !=
              std::string::npos,
      "disabled cut capability missing");

  auto reply = service.Handle("NCUT", execute().c_str());
  require(
      reply.replyCode == 557 && callback.calls == 1 &&
          callback.last.recordingKey ==
              "0123456789abcdef0123456789abcdef" &&
          callback.last.expectedMarksRevision ==
              "fedcba9876543210fedcba9876543210",
      "cut dispatch failed");

  reply = service.Handle("NCUT", execute().c_str());
  require(
      reply.replyCode == 557 && callback.calls == 1,
      "terminal cut replay dispatched twice");

  reply = service.Handle(
      "NCUT",
      execute("cmd_1", "op_1", "2").c_str());
  require(
      reply.replyCode == 559 && callback.calls == 1,
      "changed cut replay was not fenced");

  reply = service.Handle(
      "NCUT",
      execute("cmd_1", "op_2").c_str());
  require(
      reply.replyCode == 559 && callback.calls == 1,
      "cut command reuse across operations was not fenced");

  reply = service.Handle(
      "NCUT",
      execute("cmd_stale", "op_stale", "1",
              "fedcba9876543210fedcba9876543210", "pie_old").c_str());
  require(reply.replyCode == 555, "stale cut provider epoch accepted");

  reply = disabled.Handle(
      "NCUT",
      execute("cmd_disabled", "op_disabled").c_str());
  require(reply.replyCode == 556, "disabled cut accepted");

  ThrowingCallback throwingCallback;
  SuiteBridgeRecordingCutService throwing("pie_1", &throwingCallback);
  reply = throwing.Handle(
      "NCUT",
      execute("cmd_throw", "op_throw").c_str());
  require(
      reply.replyCode == 558,
      "throwing cut callback did not become outcome_unknown");

  Callback boundedCallback;
  SuiteBridgeRecordingCutService bounded("pie_1", &boundedCallback, 1);
  reply = bounded.Handle(
      "NCUT",
      execute("cmd_a", "op_a").c_str());
  require(reply.replyCode == 557, "first bounded cut replay entry failed");
  reply = bounded.Handle(
      "NCUT",
      execute("cmd_b", "op_b").c_str());
  require(reply.replyCode == 560, "bounded cut replay ledger overflow not fenced");

  require(
      !service.Handle("OTHER", "CAP 1 start").handled,
      "foreign cut command handled");

  std::cout << "suitebridge recording cut tests passed\n";
}
