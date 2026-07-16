#include "suitebridge.h"

#include <vdr/tools.h>

static const char *VERSION = "0.1.0";
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
  isyslog("suitebridge: initialize version %s", VERSION);
  return true;
}

bool cPluginSuiteBridge::Start(void)
{
  isyslog("suitebridge: started");
  return true;
}

void cPluginSuiteBridge::Stop(void)
{
  isyslog("suitebridge: stopped");
}

const char *cPluginSuiteBridge::MainMenuEntry(void)
{
  return nullptr;
}

VDRPLUGINCREATOR(cPluginSuiteBridge);
