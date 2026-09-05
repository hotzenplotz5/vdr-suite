#include "../suitebridge_recording_marks_modify.h"

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
struct Callback final : ISuiteBridgeRecordingMarksModifyMutationCallback {
  int calls = 0;
  SuiteBridgeRecordingMarksModifyRequest last;

  SuiteBridgeRecordingMarksModifyMutationResult ModifyMarks(
      const SuiteBridgeRecordingMarksModifyRequest &request) override
  {
    ++calls;
    last = request;
    return {
        SuiteBridgeRecordingMarksModifyMutationDisposition::AppliedUnverified,
        "nmarks:test:applied"};
  }
};

struct ThrowingCallback final : ISuiteBridgeRecordingMarksModifyMutationCallback {
  SuiteBridgeRecordingMarksModifyMutationResult ModifyMarks(
      const SuiteBridgeRecordingMarksModifyRequest &) override
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
    const std::string &kind = "add",
    const std::string &sourceFrame = "-",
    const std::string &targetFrame = "125",
    const std::string &replacementFrames = "-",
    const std::string &commandId = "cmd_1",
    const std::string &operationId = "op_1",
    const std::string &providerEpoch = "pie_1")
{
  return std::string("EXEC vdr-suite-native/1 vdr.recording.marks.modify 2 ") +
      commandId +
      " sha256:aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa " +
      operationId +
      " 1 0123456789abcdef0123456789abcdef fedcba9876543210fedcba9876543210 " +
      kind + " " + sourceFrame + " " + targetFrame + " " + replacementFrames +
      " job_1 attempt_1 1 backend_1 agent_1 instance_1 1 100"+
      " vdr.recording.marks suitebridge:local suitebridge 1 " + providerEpoch +
      " 1 2 vdr.recording.marks.modify 101";
}
} // namespace

int main()
{
  Callback callback;
  SuiteBridgeRecordingMarksModifyService service("pie_1", &callback);

  auto capability = service.Handle("NMARKS", "CAP 2 modify");
  require(
      capability.handled && capability.replyCode == 900 &&
          capability.payload.find(
              "vdr.recording.marks.modify 2 recording-marks-modify enabled") !=
              std::string::npos,
      "enabled capability missing");

  SuiteBridgeRecordingMarksModifyService disabled("pie_1");
  capability = disabled.Handle("NMARKS", "CAP 2 modify");
  require(
      capability.replyCode == 900 &&
          capability.payload.find("recording-marks-modify disabled") !=
              std::string::npos,
      "disabled capability missing");

  auto reply = service.Handle("NMARKS", execute().c_str());
  require(
      reply.replyCode == 557 && callback.calls == 1 &&
          callback.last.kind == SuiteBridgeRecordingMarksModifyKind::Add &&
          callback.last.sourceFrame == -1 && callback.last.targetFrame == 125 &&
          callback.last.replacementFrames.empty(),
      "ADD dispatch failed");

  reply = service.Handle("NMARKS", execute().c_str());
  require(
      reply.replyCode == 557 && callback.calls == 1,
      "terminal replay mutated twice");

  reply = service.Handle(
      "NMARKS",
      execute("add", "-", "126").c_str());
  require(
      reply.replyCode == 559 && callback.calls == 1,
      "same operation with changed canonical request was not fenced");

  reply = service.Handle(
      "NMARKS",
      execute("add", "-", "125", "-", "cmd_1", "op_2").c_str());
  require(
      reply.replyCode == 559 && callback.calls == 1,
      "command reuse across operations was not fenced");

  reply = service.Handle(
      "NMARKS",
      execute("add", "1", "125", "-", "cmd_bad", "op_bad").c_str());
  require(reply.replyCode == 501, "invalid ADD frame shape accepted");

  reply = service.Handle(
      "NMARKS",
      execute("replace", "-", "-", "125,250,375", "cmd_replace", "op_replace").c_str());
  require(
      reply.replyCode == 557 && callback.calls == 2 &&
          callback.last.kind == SuiteBridgeRecordingMarksModifyKind::Replace &&
          callback.last.replacementFrames.size() == 3 &&
          callback.last.replacementFrames[0] == 125 &&
          callback.last.replacementFrames[2] == 375,
      "REPLACE dispatch failed");

  reply = service.Handle(
      "NMARKS",
      execute("replace", "-", "-", "-", "cmd_bad_replace", "op_bad_replace").c_str());
  require(reply.replyCode == 501, "empty REPLACE accepted");

  reply = service.Handle(
      "NMARKS",
      execute("move", "125", "250", "-", "cmd_stale", "op_stale", "pie_old").c_str());
  require(reply.replyCode == 555, "stale provider epoch accepted");

  reply = disabled.Handle(
      "NMARKS",
      execute("reset", "-", "-", "-", "cmd_disabled", "op_disabled").c_str());
  require(reply.replyCode == 556, "disabled mutation accepted");

  ThrowingCallback throwingCallback;
  SuiteBridgeRecordingMarksModifyService throwing("pie_1", &throwingCallback);
  reply = throwing.Handle(
      "NMARKS",
      execute("delete", "125", "-", "-", "cmd_throw", "op_throw").c_str());
  require(reply.replyCode == 558, "throwing callback did not become outcome_unknown");

  Callback boundedCallback;
  SuiteBridgeRecordingMarksModifyService bounded("pie_1", &boundedCallback, 1);
  reply = bounded.Handle(
      "NMARKS",
      execute("reset", "-", "-", "-", "cmd_a", "op_a").c_str());
  require(reply.replyCode == 557, "first bounded replay entry failed");
  reply = bounded.Handle(
      "NMARKS",
      execute("reset", "-", "-", "-", "cmd_b", "op_b").c_str());
  require(reply.replyCode == 560, "bounded replay ledger overflow not fenced");

  require(
      !service.Handle("OTHER", "CAP 2 modify").handled,
      "foreign command handled");

  std::cout << "suitebridge recording marks modify tests passed\n";
}
