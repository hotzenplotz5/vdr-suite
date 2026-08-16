#include "suitebridge.h"

#include "suitebridge_capabilities.h"
#include "suitebridge_plugin_identity.h"

#include <vdr/tools.h>

cPluginSuiteBridge::cPluginSuiteBridge()
    : nativeProbe_(GenerateSuiteBridgePluginInstanceEpoch()),
      nativeTimerDelete_(
          nativeProbe_.PluginInstanceEpoch(),
          &nativeTimerDeleteVdrMutation_)
{
}

cPluginSuiteBridge::~cPluginSuiteBridge() = default;

const char *cPluginSuiteBridge::Version(void)
{
  return SuiteBridgePluginIdentity::Version;
}

const char *cPluginSuiteBridge::Description(void)
{
  return SuiteBridgePluginIdentity::Description;
}

bool cPluginSuiteBridge::Initialize(void)
{
  if (!lifecycle_.Initialize()) {
    esyslog(
        "suitebridge: lifecycle event=initialize result=rejected state=%s",
        lifecycle_.StateName());
    return false;
  }

  isyslog(
      "suitebridge: lifecycle event=initialize result=accepted state=%s version=%s",
      lifecycle_.StateName(),
      SuiteBridgePluginIdentity::Version);

  for (const auto &capability : SuiteBridgeCapabilities::All()) {
    isyslog(
        "suitebridge: capability schema=%u id=%s state=%s",
        SuiteBridgeCapabilities::SchemaVersion(),
        capability.id,
        SuiteBridgeCapabilities::StateName(capability.state));
  }

  isyslog(
      "suitebridge: native-operation=vdr.native.probe schema=1 side-effect=none mutations=disabled provider=suitebridge");
  isyslog(
      "suitebridge: native-operation=vdr.timer.delete schema=1 side-effect=timer-delete mutations=enabled execution=enabled provider=suitebridge acceptance=required");
  return true;
}

bool cPluginSuiteBridge::Start(void)
{
  if (!lifecycle_.Start()) {
    esyslog(
        "suitebridge: lifecycle event=start result=rejected state=%s",
        lifecycle_.StateName());
    return false;
  }

  statusMonitor_.Activate();

  isyslog(
      "suitebridge: lifecycle event=start result=accepted state=%s version=%s",
      lifecycle_.StateName(),
      SuiteBridgePluginIdentity::Version);
  return true;
}

void cPluginSuiteBridge::Stop(void)
{
  if (!lifecycle_.BeginStop()) {
    esyslog(
        "suitebridge: lifecycle event=stop-begin result=rejected state=%s version=%s",
        lifecycle_.StateName(),
        SuiteBridgePluginIdentity::Version);
    return;
  }

  if (lifecycle_.State() == SuiteBridgeLifecycleState::Stopping) {
    isyslog(
        "suitebridge: lifecycle event=stop-begin result=accepted state=%s version=%s",
        lifecycle_.StateName(),
        SuiteBridgePluginIdentity::Version);

    statusMonitor_.Deactivate();

    if (!lifecycle_.CompleteStop()) {
      esyslog(
          "suitebridge: lifecycle event=stop-complete result=rejected state=%s version=%s",
          lifecycle_.StateName(),
          SuiteBridgePluginIdentity::Version);
      return;
    }
  }

  isyslog(
      "suitebridge: lifecycle event=stop-complete result=accepted state=%s version=%s",
      lifecycle_.StateName(),
      SuiteBridgePluginIdentity::Version);
}

const char *cPluginSuiteBridge::MainMenuEntry(void)
{
  return nullptr;
}

VDRPLUGINCREATOR(cPluginSuiteBridge);
