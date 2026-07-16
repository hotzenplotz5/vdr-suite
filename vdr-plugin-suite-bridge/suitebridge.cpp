#include "suitebridge.h"

#include "suitebridge_capabilities.h"
#include "suitebridge_svdrp_contract.h"

#include <vdr/tools.h>

static const char *VERSION = "0.8.0";
static const char *DESCRIPTION =
    "Native bridge between VDR and the VDR-Suite Backend Agent";

cPluginSuiteBridge::cPluginSuiteBridge() = default;

cPluginSuiteBridge::~cPluginSuiteBridge() = default;

const char *cPluginSuiteBridge::Version(void)
{
  return VERSION;
}

const char *cPluginSuiteBridge::Description(void)
{
  return DESCRIPTION;
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
      VERSION);

  for (const auto &capability : SuiteBridgeCapabilities::All()) {
    isyslog(
        "suitebridge: capability schema=%u id=%s state=%s",
        SuiteBridgeCapabilities::SchemaVersion(),
        capability.id,
        SuiteBridgeCapabilities::StateName(capability.state));
  }

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
      VERSION);
  return true;
}

void cPluginSuiteBridge::Stop(void)
{
  if (!lifecycle_.BeginStop()) {
    esyslog(
        "suitebridge: lifecycle event=stop-begin result=rejected state=%s version=%s",
        lifecycle_.StateName(),
        VERSION);
    return;
  }

  if (lifecycle_.State() == SuiteBridgeLifecycleState::Stopping) {
    isyslog(
        "suitebridge: lifecycle event=stop-begin result=accepted state=%s version=%s",
        lifecycle_.StateName(),
        VERSION);

    statusMonitor_.Deactivate();

    if (!lifecycle_.CompleteStop()) {
      esyslog(
          "suitebridge: lifecycle event=stop-complete result=rejected state=%s version=%s",
          lifecycle_.StateName(),
          VERSION);
      return;
    }
  }

  isyslog(
      "suitebridge: lifecycle event=stop-complete result=accepted state=%s version=%s",
      lifecycle_.StateName(),
      VERSION);
}

const char *cPluginSuiteBridge::MainMenuEntry(void)
{
  return nullptr;
}

const char **cPluginSuiteBridge::SVDRPHelpPages(void)
{
  static const char *HelpPages[] = {
      "SNAP\n"
      "    Return the current read-only VDR-Suite status payload.",
      nullptr,
  };

  return HelpPages;
}

cString cPluginSuiteBridge::SVDRPCommand(
    const char *Command,
    const char *Option,
    int &ReplyCode)
{
  const SuiteBridgeSvdrpReply reply(
      Command,
      Option,
      SuiteBridgeCapabilities::SchemaVersion(),
      statusMonitor_.CaptureSnapshot());

  if (!reply.Handled()) {
    return nullptr;
  }

  ReplyCode = reply.ReplyCode();

  if (!reply.HasPayload()) {
    esyslog(
        "suitebridge: svdrp command=SNAP result=rejected reply=%d",
        ReplyCode);
    return cString::sprintf("%s", reply.Data());
  }

  isyslog(
      "suitebridge: svdrp command=SNAP result=served reply=%d bytes=%zu",
      ReplyCode,
      reply.Size());

  return cString::sprintf("%s", reply.Data());
}

VDRPLUGINCREATOR(cPluginSuiteBridge);
