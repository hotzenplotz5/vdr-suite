#include "suitebridge_epg_command_handler.h"

#include "suitebridge_epg_type_snapshot_contract.h"
#include "suitebridge_plugin_identity.h"
#include "suitebridge_tvscraper_adapter.h"

#include <vdr/channels.h>
#include <vdr/epg.h>
#include <vdr/tools.h>

#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace {

class SuiteBridgeTypeSnapshotEvent final {
public:
  SuiteBridgeTypeSnapshotEvent(
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

struct CapturedTypeEvent final {
  std::string channelId;
  std::unique_ptr<SuiteBridgeTypeSnapshotEvent> snapshot;
};

std::string Usage()
{
  return std::string("Usage: PLUG ") + SuiteBridgePluginIdentity::Name +
      " ETYPES <from-epoch> <until-epoch> <offset> <limit>";
}

SuiteBridgeEpgTypeSnapshotPage CaptureTypeSnapshot(
    const SuiteBridgeEpgTypeSnapshotRequest &request)
{
  SuiteBridgeEpgTypeSnapshotPage page;
  page.nextOffset = request.Offset();
  page.done = true;

  std::vector<CapturedTypeEvent> captured;
  captured.reserve(request.Limit());

  std::uint64_t windowOffset = 0;
  bool moreEvents = false;
  {
    LOCK_CHANNELS_READ;
    LOCK_SCHEDULES_READ;

    for (const cChannel *channel = Channels->First();
         channel != nullptr && !moreEvents;
         channel = Channels->Next(channel)) {
      if (channel->GroupSep()) {
        continue;
      }

      const cSchedule *schedule = Schedules->GetSchedule(channel);
      if (schedule == nullptr || schedule->Events() == nullptr) {
        continue;
      }

      const tChannelID channelId = channel->GetChannelID();
      const cString channelIdText = channelId.ToString();
      if (!*channelIdText) {
        continue;
      }

      for (const cEvent *event = schedule->Events()->First();
           event != nullptr;
           event = schedule->Events()->Next(event)) {
        if (event->EndTime() <= request.FromTime() ||
            event->StartTime() >= request.UntilTime()) {
          continue;
        }

        if (windowOffset < request.Offset()) {
          ++windowOffset;
          continue;
        }

        if (captured.size() >= request.Limit()) {
          moreEvents = true;
          break;
        }

        CapturedTypeEvent entry;
        entry.channelId = *channelIdText;
        entry.snapshot = std::make_unique<SuiteBridgeTypeSnapshotEvent>(
            channelId,
            *event);
        captured.push_back(std::move(entry));
        ++windowOffset;
      }
    }
  }

  page.scanned = captured.size();
  page.nextOffset = request.Offset() + page.scanned;
  page.done = !moreEvents;

  const SuiteBridgeTvScraperAdapter adapter;
  for (const CapturedTypeEvent &capturedEvent : captured) {
    const cEvent &event = capturedEvent.snapshot->Event();
    const SuiteBridgeEpgMediaType mediaType =
        adapter.ResolveMediaType(event);
    if (mediaType == SuiteBridgeEpgMediaType::None) {
      continue;
    }

    SuiteBridgeEpgTypeSnapshotItem item;
    item.channelId = capturedEvent.channelId;
    item.eventId = event.EventID();
    item.startTime = event.StartTime();
    item.endTime = event.EndTime();
    item.mediaType = mediaType;
    page.items.push_back(std::move(item));
  }

  return page;
}

} // namespace

SuiteBridgeCommandResult SuiteBridgeEpgCommandHandler::HandleTypeSnapshot(
    const char *command,
    const char *option)
{
  const SuiteBridgeEpgTypeSnapshotRequest request(command, option);
  if (!request.Handled()) {
    return {};
  }

  SuiteBridgeCommandResult result;
  result.handled = true;
  if (!request.Valid()) {
    result.replyCode = 501;
    result.payload = Usage();
    return result;
  }

  const SuiteBridgeEpgTypeSnapshotPage page = CaptureTypeSnapshot(request);
  const SuiteBridgeEpgTypeSnapshotPayload payload(page);
  if (!payload.Complete()) {
    result.replyCode = 451;
    result.payload = "EPG type snapshot payload exceeds contract capacity";
    return result;
  }

  result.replyCode = 250;
  result.payload.assign(payload.Data(), payload.Size());
  isyslog(
      "suitebridge: svdrp command=ETYPES result=served offset=%llu scanned=%zu found=%zu done=%s bytes=%zu",
      static_cast<unsigned long long>(request.Offset()),
      page.scanned,
      page.items.size(),
      page.done ? "true" : "false",
      payload.Size());
  return result;
}
