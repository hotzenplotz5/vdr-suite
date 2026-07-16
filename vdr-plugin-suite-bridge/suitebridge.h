#ifndef VDR_SUITE_BRIDGE_H
#define VDR_SUITE_BRIDGE_H

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
};

#endif
