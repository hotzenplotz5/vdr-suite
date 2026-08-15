#include "suitebridge_native_timer_delete.h"

#include <cassert>
#include <string>

int main()
{
  SuiteBridgeNativeTimerDeleteService service("pie_1");

  {
    const SuiteBridgeCommandResult reply = service.Handle("NTDEL", "CAP 1");
    assert(reply.handled);
    assert(reply.replyCode == 900);
    assert(reply.payload ==
        "vdr-suite-ntdel-cap/1 vdr.timer.delete 1 timer-delete disabled "
        "suitebridge pie_1 1 1 disabled");
  }

  const std::string request =
      "EXEC vdr-suite-native/1 vdr.timer.delete 1 "
      "cmd_1 fp_1 op_1 opr_1 ntb_1 nbr_1 tas_1 42 job_1 att_1 3 "
      "default agt_1 agi_1 7 100 vdr.timer suitebridge:local suitebridge "
      "9 pie_1 1 1 vdr.timer.delete 101";

  {
    const SuiteBridgeCommandResult reply = service.Handle("NTDEL", request.c_str());
    assert(reply.handled);
    assert(reply.replyCode == 556);
    assert(reply.payload ==
        "vdr-suite-ntdel-result/1 cmd_1 fp_1 vdr.timer.delete 1 pie_1 1 1 "
        "rejected_without_effect disabled ntdel:disabled:cmd_1");
  }

  {
    std::string stale = request;
    const std::size_t position = stale.find(" pie_1 1 1 vdr.timer.delete");
    assert(position != std::string::npos);
    stale.replace(position + 1, 5, "pie_0");
    const SuiteBridgeCommandResult reply = service.Handle("NTDEL", stale.c_str());
    assert(reply.handled);
    assert(reply.replyCode == 555);
    assert(reply.payload ==
        "vdr-suite-ntdel-result/1 cmd_1 fp_1 vdr.timer.delete 1 pie_1 1 1 "
        "rejected_without_effect disabled ntdel:stale:cmd_1");
  }

  {
    const SuiteBridgeCommandResult reply = service.Handle(
        "NTDEL", "EXEC vdr-suite-native/1 vdr.timer.delete 1 broken");
    assert(reply.handled);
    assert(reply.replyCode == 501);
    assert(reply.payload == "vdr-suite-ntdel-rejected/1 execute-malformed");
  }

  {
    const SuiteBridgeCommandResult reply = service.Handle("SNAP", nullptr);
    assert(!reply.handled);
  }

  return 0;
}
