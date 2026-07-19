#include "suitebridge.h"

#include "suitebridge_capabilities.h"
#include "suitebridge_capability_discovery.h"
#include "suitebridge_epg_artwork_contract.h"
#include "suitebridge_svdrp_contract.h"
#include "suitebridge_tvscraper_adapter.h"

#include <vdr/channels.h>
#include <vdr/epg.h>
#include <vdr/tools.h>

static const char *PLUGIN_NAME = "suitebridge";
static const char *VERSION = "0.10.0";
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
      "CAPS [discovery-schema]\n"
      "    Return the read-only VDR-Suite capability discovery payload.",
      "SNAP\n"
      "    Return the current read-only VDR-Suite status payload.",
      "ARTW <channel-id> <event-id>\n"
      "    Resolve preferred TVScraper artwork for one EPG event.",
      nullptr,
  };

  return HelpPages;
}

cString cPluginSuiteBridge::SVDRPCommand(
    const char *Command,
    const char *Option,
    int &ReplyCode)
{
  const SuiteBridgeCapabilityDiscoveryReply capabilityReply(
      Command,
      Option,
      PLUGIN_NAME,
      VERSION);

  if (capabilityReply.Handled()) {
    ReplyCode = capabilityReply.ReplyCode();

    if (!capabilityReply.HasPayload()) {
      esyslog(
          "suitebridge: svdrp command=CAPS result=rejected reply=%d",
          ReplyCode);
      return cString::sprintf("%s", capabilityReply.Data());
    }

    isyslog(
        "suitebridge: svdrp command=CAPS result=served reply=%d bytes=%zu schema=%u",
        ReplyCode,
        capabilityReply.Size(),
        SuiteBridgeCapabilityDiscoveryPayload::SchemaVersion());

    return cString::sprintf("%s", capabilityReply.Data());
  }

  const SuiteBridgeEpgArtworkRequest artworkRequest(Command, Option);
  if (artworkRequest.Handled()) {
    if (!artworkRequest.Valid()) {
      ReplyCode = 501;
      return cString::sprintf(
          "Usage: PLUG %s ARTW <channel-id> <event-id>",
          PLUGIN_NAME);
    }

    SuiteBridgeArtworkReference artwork;
    const tChannelID channelId =
        tChannelID::FromString(artworkRequest.ChannelId().c_str());

    if (channelId.Valid()) {
      LOCK_SCHEDULES_READ;
      const cSchedule *schedule = Schedules->GetSchedule(channelId);
      const cEvent *event = nullptr;

      if (schedule) {
#if APIVERSNUM >= 20502
        event = schedule->GetEventById(artworkRequest.EventId());
#else
        event = schedule->GetEvent(artworkRequest.EventId());
#endif
      }

      if (event) {
        const SuiteBridgeTvScraperAdapter adapter;
        artwork = adapter.ResolvePreferredArtwork(*event);
      }
    }

    const SuiteBridgeEpgArtworkPayload payload(artwork);
    if (!payload.Complete()) {
      ReplyCode = 451;
      esyslog(
          "suitebridge: svdrp command=ARTW result=overflow channel=%s event=%u",
          artworkRequest.ChannelId().c_str(),
          artworkRequest.EventId());
      return cString::sprintf("Artwork payload exceeds contract capacity");
    }

    ReplyCode = 250;
    isyslog(
        "suitebridge: svdrp command=ARTW result=served channel=%s event=%u found=%s bytes=%zu",
        artworkRequest.ChannelId().c_str(),
        artworkRequest.EventId(),
        artwork.Valid() ? "true" : "false",
        payload.Size());
    return cString::sprintf("%s", payload.Data());
  }

  const SuiteBridgeSvdrpReply snapshotReply(
      Command,
      Option,
      SuiteBridgeCapabilities::SchemaVersion(),
      statusMonitor_.CaptureSnapshot());

  if (!snapshotReply.Handled()) {
    return nullptr;
  }

  ReplyCode = snapshotReply.ReplyCode();

  if (!snapshotReply.HasPayload()) {
    esyslog(
        "suitebridge: svdrp command=SNAP result=rejected reply=%d",
        ReplyCode);
    return cString::sprintf("%s", snapshotReply.Data());
  }

  isyslog(
      "suitebridge: svdrp command=SNAP result=served reply=%d bytes=%zu",
      ReplyCode,
      snapshotReply.Size());

  return cString::sprintf("%s", snapshotReply.Data());
}

VDRPLUGINCREATOR(cPluginSuiteBridge);
