#include "../suitebridge_native_timer_modify.h"

#include <cstdlib>
#include <iostream>
#include <string>

namespace {
struct Callback final : ISuiteBridgeNativeTimerModifyMutationCallback {
  int calls=0;
  SuiteBridgeNativeTimerModifyRequest last;
  SuiteBridgeNativeTimerModifyMutationResult ModifyTimer(
      const SuiteBridgeNativeTimerModifyRequest &request) override {
    ++calls; last=request;
    return {SuiteBridgeNativeTimerModifyMutationDisposition::AppliedUnverified,
            "ntmod:test:applied"};
  }
};
void require(bool value,const char *message) {
  if (!value) { std::cerr << message << '\n'; std::exit(1); }
}
std::string execute(const char *operation,const char *capability) {
  return std::string("EXEC vdr-suite-native/1 ")+operation+
      " 1 cmd_1 sha256:aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"+
      " op_1 1 bind_1 1 sha256:bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb"+
      " assign_1 42 532d31312e38382d30 5469746c65 - 323032362d30382d3137"+
      " 2d2d2d2d2d2d2d 31323030 31333030 50 99 1 0 job_1 attempt_1 1"+
      " backend_1 agent_1 instance_1 1 100 vdr.timer suitebridge:local suitebridge"+
      " 1 pie_1 1 1 "+capability+" 101";
}
}
int main() {
  Callback callback;
  SuiteBridgeNativeTimerModifyService service("pie_1",&callback);
  auto cap=service.Handle("NTMOD","CAP 1 update");
  require(cap.handled && cap.replyCode==900 &&
      cap.payload.find("vdr.timer.update 1 timer-modify enabled")!=std::string::npos,
      "UPDATE capability missing");
  cap=service.Handle("NTMOD","CAP 1 toggle");
  require(cap.replyCode==900 && cap.payload.find("vdr.timer.toggle")!=std::string::npos,
      "TOGGLE capability missing");
  auto reply=service.Handle("NTMOD",execute("vdr.timer.update","vdr.timer.update").c_str());
  require(reply.replyCode==557 && callback.calls==1 &&
      callback.last.kind==SuiteBridgeNativeTimerModifyKind::Update,
      "UPDATE dispatch failed");
  reply=service.Handle("NTMOD",execute("vdr.timer.update","vdr.timer.update").c_str());
  require(reply.replyCode==557 && callback.calls==1,"terminal replay mutated twice");
  reply=service.Handle("NTMOD",execute("vdr.timer.toggle","vdr.timer.toggle").c_str());
  require(reply.replyCode==559 && callback.calls==1,"operation replay conflict not fenced");
  SuiteBridgeNativeTimerModifyService disabled("pie_1");
  reply=disabled.Handle("NTMOD",execute("vdr.timer.toggle","vdr.timer.toggle").c_str());
  require(reply.replyCode==556,"disabled mutation accepted");
  require(!service.Handle("OTHER","CAP 1 update").handled,"foreign command handled");
  std::cout << "suitebridge native timer modify tests passed\n";
}
