#include "suitebridge_epg_command_handler.h"

#include "suitebridge_epg_artwork_contract.h"
#include "suitebridge_epg_metadata_contract.h"
#include "suitebridge_plugin_identity.h"
#include "suitebridge_tvscraper_adapter.h"

#include <vdr/channels.h>
#include <vdr/epg.h>
#include <vdr/tools.h>

#include <memory>
#include <string>

namespace {

class SuiteBridgeDetachedEventSnapshot final {
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

std::string Usage(const char *command)
{
  return std::string("Usage: PLUG ") + SuiteBridgePluginIdentity::Name +
      " " + command + " <channel-id> <event-id>";
}

} // namespace

SuiteBridgeCommandResult SuiteBridgeEpgCommandHandler::HandleArtwork(
    const char *command,
    const char *option)
{
  const SuiteBridgeEpgArtworkRequest request(command, option);
  if (!request.Handled()) {
    return {};
  }

  SuiteBridgeCommandResult result;
  result.handled = true;
  if (!request.Valid()) {
    result.replyCode = 501;
    result.payload = Usage("ARTW");
    return result;
  }

  SuiteBridgeArtworkReference artwork;
  const tChannelID channelId =
      tChannelID::FromString(request.ChannelId().c_str());

  if (!channelId.Valid()) {
    isyslog(
        "suitebridge: artwork lookup result=invalid-channel channel=%s event=%u",
        request.ChannelId().c_str(),
        request.EventId());
  } else {
    std::unique_ptr<SuiteBridgeDetachedEventSnapshot> snapshot =
        CaptureEpgEvent(channelId, request.EventId());

    if (!snapshot) {
      isyslog(
          "suitebridge: artwork lookup result=event-unavailable channel=%s event=%u",
          request.ChannelId().c_str(),
          request.EventId());
    } else {
      const cEvent &event = snapshot->Event();
      isyslog(
          "suitebridge: artwork lookup result=event-snapshot channel=%s event=%u title=%s",
          request.ChannelId().c_str(),
          request.EventId(),
          event.Title() ? event.Title() : "");

      const SuiteBridgeTvScraperAdapter adapter;
      artwork = adapter.ResolvePreferredArtwork(event);
    }
  }

  const SuiteBridgeEpgArtworkPayload payload(artwork);
  if (!payload.Complete()) {
    result.replyCode = 451;
    result.payload = "Artwork payload exceeds contract capacity";
    esyslog(
        "suitebridge: svdrp command=ARTW result=overflow channel=%s event=%u",
        request.ChannelId().c_str(),
        request.EventId());
    return result;
  }

  result.replyCode = 250;
  result.payload.assign(payload.Data(), payload.Size());
  isyslog(
      "suitebridge: svdrp command=ARTW result=served channel=%s event=%u found=%s bytes=%zu",
      request.ChannelId().c_str(),
      request.EventId(),
      artwork.Valid() ? "true" : "false",
      payload.Size());
  return result;
}

SuiteBridgeCommandResult SuiteBridgeEpgCommandHandler::HandleMetadata(
    const char *command,
    const char *option)
{
  const SuiteBridgeEpgMetadataRequest request(command, option);
  if (!request.Handled()) {
    return {};
  }

  SuiteBridgeCommandResult result;
  result.handled = true;
  if (!request.Valid()) {
    result.replyCode = 501;
    result.payload = Usage("META");
    return result;
  }

  SuiteBridgeEpgMetadata metadata;
  const tChannelID channelId =
      tChannelID::FromString(request.ChannelId().c_str());

  if (!channelId.Valid()) {
    isyslog(
        "suitebridge: metadata lookup result=invalid-channel channel=%s event=%u",
        request.ChannelId().c_str(),
        request.EventId());
  } else {
    std::unique_ptr<SuiteBridgeDetachedEventSnapshot> snapshot =
        CaptureEpgEvent(channelId, request.EventId());

    if (!snapshot) {
      isyslog(
          "suitebridge: metadata lookup result=event-unavailable channel=%s event=%u",
          request.ChannelId().c_str(),
          request.EventId());
    } else {
      const cEvent &event = snapshot->Event();
      isyslog(
          "suitebridge: metadata lookup result=event-snapshot channel=%s event=%u title=%s",
          request.ChannelId().c_str(),
          request.EventId(),
          event.Title() ? event.Title() : "");

      const SuiteBridgeTvScraperAdapter adapter;
      metadata = adapter.ResolveMetadata(event);
    }
  }

  try {
    const SuiteBridgeEpgMetadataPayload payload(metadata);
    if (!payload.Complete()) {
      result.replyCode = 451;
      result.payload = "Metadata payload exceeds contract capacity";
      esyslog(
          "suitebridge: svdrp command=META result=overflow channel=%s event=%u",
          request.ChannelId().c_str(),
          request.EventId());
      return result;
    }

    result.replyCode = 250;
    result.payload.assign(payload.Data(), payload.Size());
    isyslog(
        "suitebridge: svdrp command=META result=served channel=%s event=%u found=%s bytes=%zu people=%zu images=%zu",
        request.ChannelId().c_str(),
        request.EventId(),
        metadata.found ? "true" : "false",
        payload.Size(),
        metadata.people.size(),
        metadata.images.size());
    return result;
  } catch (...) {
    result.replyCode = 451;
    result.payload = "Metadata payload serialization failed";
    esyslog(
        "suitebridge: svdrp command=META result=serialization-failed channel=%s event=%u",
        request.ChannelId().c_str(),
        request.EventId());
    return result;
  }
}
