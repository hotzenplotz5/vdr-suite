#include "suitebridge.h"

#include <vdr/tools.h>

static const char *VERSION = "0.2.0";
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

  isyslog(
      "suitebridge: lifecycle event=start result=accepted state=%s version=%s",
      lifecycle_.StateName(),
      VERSION);
  return true;
}

void cPluginSuiteBridge::Stop(void)
{
  lifecycle_.Stop();
  isyslog(
      "suitebridge: lifecycle event=stop result=accepted state=%s version=%s",
      lifecycle_.StateName(),
      VERSION);
}

const char *cPluginSuiteBridge::MainMenuEntry(void)
{
  return nullptr;
}

VDRPLUGINCREATOR(cPluginSuiteBridge);
