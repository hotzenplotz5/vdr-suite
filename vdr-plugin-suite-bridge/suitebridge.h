#ifndef VDR_SUITE_BRIDGE_H
#define VDR_SUITE_BRIDGE_H

#include "suitebridge_lifecycle.h"
#include "suitebridge_native_probe.h"
#include "suitebridge_native_timer_delete.h"
#include "suitebridge_status_monitor.h"

#include <vdr/plugin.h>

class cPluginSuiteBridge final : public cPlugin {
public:
  cPluginSuiteBridge();
  ~cPluginSuiteBridge() override;

  const char *Version(void) override;
  const char *Description(void) override;

  bool Initialize(void) override;
  bool Start(void) override;
  void Stop(void) override;

  const char *MainMenuEntry(void) override;
  const char **SVDRPHelpPages(void) override;
  cString SVDRPCommand(
      const char *Command,
      const char *Option,
      int &ReplyCode) override;

private:
  SuiteBridgeLifecycle lifecycle_;
  SuiteBridgeStatusMonitor statusMonitor_;
  SuiteBridgeNativeProbeService nativeProbe_;
  SuiteBridgeNativeTimerDeleteService nativeTimerDelete_;
};

#endif
