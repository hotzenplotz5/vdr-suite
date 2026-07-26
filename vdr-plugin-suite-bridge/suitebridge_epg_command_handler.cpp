#include "suitebridge_epg_command_handler.h"

#include "suitebridge_epg_artwork_contract.h"
#include "suitebridge_epg_metadata_contract.h"
#include "suitebridge_plugin_identity.h"
#include "suitebridge_tvscraper_adapter.h"
#include "services.h"

#include <vdr/channels.h>
#include <vdr/epg.h>
#include <vdr/tools.h>

#include <cstdint>
#include <cstdio>
#include <strings.h>
#include <memory>
#include <sstream>
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

std::string JsonString(const char *value)
{
  const std::string text = value == nullptr ? std::string() : value;
  std::ostringstream output;
  output << '"';
  for (const unsigned char character : text) {
    switch (character) {
    case '"':
      output << "\\\"";
      break;
    case '\\':
      output << "\\\\";
      break;
    case '\b':
      output << "\\b";
      break;
    case '\f':
      output << "\\f";
      break;
    case '\n':
      output << "\\n";
      break;
    case '\r':
      output << "\\r";
      break;
    case '\t':
      output << "\\t";
      break;
    default:
      if (character < 0x20) {
        char escaped[7];
        std::snprintf(
            escaped,
            sizeof(escaped),
            "\\u%04x",
            static_cast<unsigned int>(character));
        output << escaped;
      } else {
        output << static_cast<char>(character);
      }
      break;
    }
  }
  output << '"';
  return output.str();
}


SuiteBridgeEpgMediaType ComparisonMediaType(tvType type) noexcept
{
  switch (type) {
  case tMovie:
    return SuiteBridgeEpgMediaType::Movie;
  case tSeries:
    return SuiteBridgeEpgMediaType::Series;
  case tNone:
    break;
  }
  return SuiteBridgeEpgMediaType::None;
}

SuiteBridgeEpgMetadata ResolveComparisonMetadata(const cEvent &event)
{
  cGetScraperVideo request(&event, nullptr);
  if (!request.call() || !request.m_scraperVideo) {
    return {};
  }

  cScraperVideo &video = *request.m_scraperVideo;
  SuiteBridgeEpgMetadata metadata;
  metadata.found = true;
  metadata.mediaType = ComparisonMediaType(video.getVideoType());
  metadata.providerId = video.getDbId();

  std::string title;
  std::string originalTitle;
  std::string tagline;
  std::string overview;
  std::string homepage;
  std::string releaseDate;
  bool adult = false;
  int runtime = 0;
  float popularity = 0.0F;
  float voteAverage = 0.0F;
  int voteCount = 0;
  std::vector<std::string> productionCountries;
  std::string imdbId;
  int budget = 0;
  int revenue = 0;
  int collectionId = 0;
  std::string collectionName;
  std::string status;
  std::vector<std::string> networks;
  int lastSeason = 0;

  // This is the exact Live 3.5.5 genre source. Keep the complete vector here
  // so the diagnostic can also detect a future difference caused by the
  // bounded public META contract.
  if (video.getMovieOrTv(
          &title,
          &originalTitle,
          &tagline,
          &overview,
          &metadata.genres,
          &homepage,
          &releaseDate,
          &adult,
          &runtime,
          &popularity,
          &voteAverage,
          &voteCount,
          &productionCountries,
          &imdbId,
          &budget,
          &revenue,
          &collectionId,
          &collectionName,
          &status,
          &networks,
          &lastSeason)) {
    return metadata;
  }

  metadata.genres.clear();
  return metadata;
}

std::string MetadataComparisonValue(
    const SuiteBridgeEpgMetadata &metadata)
{
  std::ostringstream output;
  output << "{\"found\":" << (metadata.found ? "true" : "false")
         << ",\"provider\":"
         << JsonString(metadata.found ? "tvscraper" : "none")
         << ",\"providerId\":" << metadata.providerId
         << ",\"mediaType\":"
         << JsonString(SuiteBridgeEpgMediaTypeName(metadata.mediaType))
         << ",\"genres\":[";

  for (std::size_t index = 0; index < metadata.genres.size(); ++index) {
    if (index > 0) {
      output << ',';
    }
    output << JsonString(metadata.genres[index].c_str());
  }
  output << "]}";
  return output.str();
}

std::string MetadataComparisonPayload(
    const SuiteBridgeEpgMetadataRequest &request,
    bool eventAvailable,
    const std::string &title,
    std::int64_t startTime,
    std::int64_t endTime,
    const SuiteBridgeEpgMetadata &liveMetadata,
    const SuiteBridgeEpgMetadata &detachedMetadata)
{
  std::ostringstream output;
  output << "{\"schema\":1"
         << ",\"channelId\":" << JsonString(request.ChannelId().c_str())
         << ",\"eventId\":" << request.EventId()
         << ",\"eventAvailable\":" << (eventAvailable ? "true" : "false")
         << ",\"title\":" << JsonString(title.c_str())
         << ",\"startTime\":" << startTime
         << ",\"endTime\":" << endTime
         << ",\"live\":" << MetadataComparisonValue(liveMetadata)
         << ",\"detached\":" << MetadataComparisonValue(detachedMetadata)
         << '}';
  return output.str();
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

SuiteBridgeCommandResult
SuiteBridgeEpgCommandHandler::HandleMetadataComparison(
    const char *command,
    const char *option)
{
  if (command == nullptr || strcasecmp(command, "MCOMPARE") != 0) {
    return {};
  }

  const SuiteBridgeEpgMetadataRequest request("META", option);
  SuiteBridgeCommandResult result;
  result.handled = true;
  if (!request.Valid()) {
    result.replyCode = 501;
    result.payload = Usage("MCOMPARE");
    return result;
  }

  const tChannelID channelId =
      tChannelID::FromString(request.ChannelId().c_str());
  SuiteBridgeEpgMetadata liveMetadata;
  SuiteBridgeEpgMetadata detachedMetadata;
  std::unique_ptr<SuiteBridgeDetachedEventSnapshot> detachedSnapshot;
  std::string title;
  std::int64_t startTime = 0;
  std::int64_t endTime = 0;
  bool eventAvailable = false;

  if (channelId.Valid()) {
    {
      LOCK_CHANNELS_READ;
      LOCK_SCHEDULES_READ;

      const cChannel *channel = Channels->GetByChannelID(channelId);
      const cSchedule *schedule =
          channel == nullptr ? nullptr : Schedules->GetSchedule(channel);
      const cEvent *event = nullptr;
      if (schedule != nullptr) {
#if APIVERSNUM >= 20502
        event = schedule->GetEventById(request.EventId());
#else
        event = schedule->GetEvent(request.EventId());
#endif
      }

      if (event != nullptr) {
        eventAvailable = true;
        title = event->Title() == nullptr ? std::string() : event->Title();
        startTime = static_cast<std::int64_t>(event->StartTime());
        endTime = static_cast<std::int64_t>(event->EndTime());

        // This is the relevant Live 3.5.5 call context: TVScraper receives the
        // real schedule-owned cEvent while channel and schedule locks protect
        // its identity, relationships and lifetime.
        liveMetadata = ResolveComparisonMetadata(*event);
        detachedSnapshot =
            std::make_unique<SuiteBridgeDetachedEventSnapshot>(
                channelId,
                *event);
      }
    }

    // Match the current SuiteBridge META implementation after the VDR locks
    // have been released.
    if (detachedSnapshot) {
      detachedMetadata =
          ResolveComparisonMetadata(detachedSnapshot->Event());
    }
  }

  try {
    result.payload = MetadataComparisonPayload(
        request,
        eventAvailable,
        title,
        startTime,
        endTime,
        liveMetadata,
        detachedMetadata);
    if (result.payload.size() >= 8192) {
      result.replyCode = 451;
      result.payload = "Metadata comparison payload exceeds contract capacity";
      esyslog(
          "suitebridge: svdrp command=MCOMPARE result=overflow channel=%s event=%u",
          request.ChannelId().c_str(),
          request.EventId());
      return result;
    }

    result.replyCode = 250;
    isyslog(
        "suitebridge: svdrp command=MCOMPARE result=served channel=%s event=%u available=%s live-found=%s detached-found=%s live-genres=%zu detached-genres=%zu bytes=%zu",
        request.ChannelId().c_str(),
        request.EventId(),
        eventAvailable ? "true" : "false",
        liveMetadata.found ? "true" : "false",
        detachedMetadata.found ? "true" : "false",
        liveMetadata.genres.size(),
        detachedMetadata.genres.size(),
        result.payload.size());
    return result;
  } catch (...) {
    result.replyCode = 451;
    result.payload = "Metadata comparison payload serialization failed";
    esyslog(
        "suitebridge: svdrp command=MCOMPARE result=serialization-failed channel=%s event=%u",
        request.ChannelId().c_str(),
        request.EventId());
    return result;
  }
}
