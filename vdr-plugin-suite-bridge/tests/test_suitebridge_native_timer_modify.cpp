#include "../suitebridge_native_timer_modify.h"
#include "../suitebridge_native_timer_modify_flags.h"

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
  // tfActive=1 / tfVps=4 fixtures; auxiliary bits survive UPDATE.
  const unsigned outside=0x02U | 0x08U | 0x10U | 0x20U;
  const unsigned original=outside | 0x01U | 0x04U;
  using vdrsuite_suitebridge::updatedNativeTimerFlags;
  require(updatedNativeTimerFlags(original, 0x01U, 0x04U, false, false)==outside,
      "UPDATE cleared native non-target flags");
  require(updatedNativeTimerFlags(original, 0x01U, 0x04U, false, true)==
      (outside | 0x04U), "UPDATE changed preserved flag or VPS");
  require(updatedNativeTimerFlags(outside, 0x01U, 0x04U, true, true)==original,
      "UPDATE failed to set active/VPS without dropping extra bits");
  require(updatedNativeTimerFlags(0U, 0x01U, 0x04U, true, false)==0x01U,
      "UPDATE default native flags changed");
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
  // Existing v1 remains byte-for-byte compatible. AUX writes are
  // explicit v2 requests and require an exact provider-side predecessor.
  Callback auxCallback;
  SuiteBridgeNativeTimerModifyService auxService("pie_1", &auxCallback);
  const std::string v1 = execute("vdr.timer.update", "vdr.timer.update");
  const std::size_t versionOffset = v1.find(" 1 cmd_1 ");
  require(versionOffset != std::string::npos, "v1 protocol fixture missing");
  std::string v2 = v1;
  v2.replace(versionOffset, 9, " 2 cmd_1 ");
  v2 += " 1 sha256:cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc 3c656d7074792f3e";
  reply = auxService.Handle("NTMOD", v2.c_str());
  require(reply.replyCode == 557 && auxCallback.calls == 1 &&
      auxCallback.last.modifyAux &&
      auxCallback.last.expectedAuxFingerprint ==
        "sha256:cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc" &&
      auxCallback.last.desiredAux == "<empty/>",
      "explicit v2 Aux mutation not passed to native writer");
  reply = auxService.Handle("NTMOD", v2.c_str());
  require(reply.replyCode == 557 && auxCallback.calls == 1,
      "Aux mutation replay must never write twice");
  std::string malformed = v2;
  malformed.replace(malformed.find(" 1 sha256:cccc"), 2, " 0");
  reply = auxService.Handle("NTMOD", malformed.c_str());
  require(reply.replyCode == 501, "v2 Aux omission accepted");
  SuiteBridgeNativeTimerModifyService disabled("pie_1");
  reply=disabled.Handle("NTMOD",execute("vdr.timer.toggle","vdr.timer.toggle").c_str());
  require(reply.replyCode==556,"disabled mutation accepted");
  require(!service.Handle("OTHER","CAP 1 update").handled,"foreign command handled");
  std::cout << "suitebridge native timer modify tests passed\n";
}
