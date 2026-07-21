#include "suitebridge.h"

#include "suitebridge_capabilities.h"
#include "suitebridge_capability_discovery.h"
#include "suitebridge_epg_artwork_contract.h"
#include "suitebridge_epg_metadata_contract.h"
#include "suitebridge_svdrp_contract.h"
#include "suitebridge_tvscraper_adapter.h"

#include <vdr/channels.h>
#include <vdr/epg.h>
#include <vdr/tools.h>

#include <memory>

static const char *PLUGIN_NAME = "suitebridge";
static const char *VERSION = "0.11.0";
static const char *DESCRIPTION =
    "Native bridge between VDR and the VDR-Suite Backend Agent";

namespace {

class SuiteBridgeDetachedEventSnapshot {
public:
  SuiteBridgeDetachedEventSnapshot(
      const tChannelID &channelId,
      const cEvent &source)
      : schedule_(channelId),
        event_(new cEvent(source.EventID()))
  {
    event_->SetTableID(source.TableID());
    event_->SetVersion(source.Version());
    event_->SetTitle(source.Title());
    event_->SetShortText(source.ShortText());
    event_->SetDescription(source.Description());

    uchar contents[MaxEventContents];
    for (int index = 0; index < MaxEventContents; ++index) {
      contents[index] = source.Contents(index);
    }
    event_->SetContents(contents);

    event_->SetParentalRating(source.ParentalRating());
    event_->SetStartTime(source.StartTime());
    event_->SetDuration(source.Duration());
    event_->SetVps(source.Vps());
    event_->SetAux(source.Aux());

    schedule_.AddEvent(event_);
  }

  const cEvent &Event() const noexcept
  {
    return *event_;
  }

private:
  cSchedule schedule_;
  cEvent *event_;
};

std::unique_ptr<SuiteBridgeDetachedEventSnapshot> CaptureEpgEvent(
    const tChannelID &channelId,
    unsigned int eventId)
{
  LOCK_SCHEDULES_READ;
  const cSchedule *schedule = Schedules->GetSchedule(channelId);
  if (!schedule) {
    return nullptr;
  }

  const cEvent *event = nullptr;
#if APIVERSNUM >= 20502
  event = schedule->GetEventById(eventId);
#else
  event = schedule->GetEvent(eventId);
#endif

  if (!event) {
    return nullptr;
  }

  return std::make_unique<SuiteBridgeDetachedEventSnapshot>(
      channelId,
      *event);
}

} // namespace

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
      "META <channel-id> <event-id>\n"
      "    Resolve bounded TVScraper metadata for one EPG event.",
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

    if (!channelId.Valid()) {
      isyslog(
          "suitebridge: artwork lookup result=invalid-channel channel=%s event=%u",
          artworkRequest.ChannelId().c_str(),
          artworkRequest.EventId());
    } else {
      std::unique_ptr<SuiteBridgeDetachedEventSnapshot> eventSnapshot =
          CaptureEpgEvent(channelId, artworkRequest.EventId());

      if (!eventSnapshot) {
        isyslog(
            "suitebridge: artwork lookup result=event-unavailable channel=%s event=%u",
            artworkRequest.ChannelId().c_str(),
            artworkRequest.EventId());
      } else {
        const cEvent &event = eventSnapshot->Event();
        isyslog(
            "suitebridge: artwork lookup result=event-snapshot channel=%s event=%u title=%s",
            artworkRequest.ChannelId().c_str(),
            artworkRequest.EventId(),
            event.Title() ? event.Title() : "");

        const SuiteBridgeTvScraperAdapter adapter;
        artwork = adapter.ResolvePreferredArtwork(event);
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

  const SuiteBridgeEpgMetadataRequest metadataRequest(Command, Option);
  if (metadataRequest.Handled()) {
    if (!metadataRequest.Valid()) {
      ReplyCode = 501;
      return cString::sprintf(
          "Usage: PLUG %s META <channel-id> <event-id>",
          PLUGIN_NAME);
    }

    SuiteBridgeEpgMetadata metadata;
    const tChannelID channelId =
        tChannelID::FromString(metadataRequest.ChannelId().c_str());

    if (!channelId.Valid()) {
      isyslog(
          "suitebridge: metadata lookup result=invalid-channel channel=%s event=%u",
          metadataRequest.ChannelId().c_str(),
          metadataRequest.EventId());
    } else {
      std::unique_ptr<SuiteBridgeDetachedEventSnapshot> eventSnapshot =
          CaptureEpgEvent(channelId, metadataRequest.EventId());

      if (!eventSnapshot) {
        isyslog(
            "suitebridge: metadata lookup result=event-unavailable channel=%s event=%u",
            metadataRequest.ChannelId().c_str(),
            metadataRequest.EventId());
      } else {
        const cEvent &event = eventSnapshot->Event();
        isyslog(
            "suitebridge: metadata lookup result=event-snapshot channel=%s event=%u title=%s",
            metadataRequest.ChannelId().c_str(),
            metadataRequest.EventId(),
            event.Title() ? event.Title() : "");

        const SuiteBridgeTvScraperAdapter adapter;
        metadata = adapter.ResolveMetadata(event);
      }
    }

    try {
      const SuiteBridgeEpgMetadataPayload payload(metadata);
      if (!payload.Complete()) {
        ReplyCode = 451;
        esyslog(
            "suitebridge: svdrp command=META result=overflow channel=%s event=%u",
            metadataRequest.ChannelId().c_str(),
            metadataRequest.EventId());
        return cString::sprintf(
            "Metadata payload exceeds contract capacity");
      }

      ReplyCode = 250;
      isyslog(
          "suitebridge: svdrp command=META result=served channel=%s event=%u found=%s bytes=%zu people=%zu images=%zu",
          metadataRequest.ChannelId().c_str(),
          metadataRequest.EventId(),
          metadata.found ? "true" : "false",
          payload.Size(),
          metadata.people.size(),
          metadata.images.size());
      return cString::sprintf("%s", payload.Data());
    } catch (...) {
      ReplyCode = 451;
      esyslog(
          "suitebridge: svdrp command=META result=serialization-failed channel=%s event=%u",
          metadataRequest.ChannelId().c_str(),
          metadataRequest.EventId());
      return cString::sprintf("Metadata payload serialization failed");
    }
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
