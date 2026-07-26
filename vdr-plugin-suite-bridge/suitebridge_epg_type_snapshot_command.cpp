#include "suitebridge_epg_command_handler.h"

#include "suitebridge_epg_type_snapshot_contract.h"
#include "suitebridge_plugin_identity.h"
#include "suitebridge_tvscraper_adapter.h"

#include <vdr/channels.h>
#include <vdr/epg.h>
#include <vdr/tools.h>

#include <algorithm>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

namespace {

constexpr std::size_t MaximumRetainedWindows = 4;
constexpr std::size_t MaximumSnapshotEvents = 100000;

struct StableTypeEventIdentity final {
  tChannelID channelId;
  std::string channelIdText;
  unsigned int eventId = 0;
  std::int64_t startTime = 0;
  std::int64_t endTime = 0;
};

struct StableTypeWindowSnapshot final {
  bool complete = true;
  std::vector<StableTypeEventIdentity> events;
};

struct RetainedTypeWindow final {
  std::int64_t fromTime = 0;
  std::int64_t untilTime = 0;
  std::uint64_t lastUsed = 0;
  std::shared_ptr<const StableTypeWindowSnapshot> snapshot;
};

std::shared_ptr<const StableTypeWindowSnapshot> BuildStableWindowSnapshot(
    const SuiteBridgeEpgTypeSnapshotRequest &request)
{
  auto snapshot = std::make_shared<StableTypeWindowSnapshot>();

  LOCK_CHANNELS_READ;
  LOCK_SCHEDULES_READ;

  for (const cChannel *channel = Channels->First();
       channel != nullptr;
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

      if (snapshot->events.size() >= MaximumSnapshotEvents) {
        snapshot->complete = false;
        return snapshot;
      }

      StableTypeEventIdentity identity;
      identity.channelId = channelId;
      identity.channelIdText = *channelIdText;
      identity.eventId = event->EventID();
      identity.startTime = event->StartTime();
      identity.endTime = event->EndTime();
      snapshot->events.push_back(std::move(identity));
    }
  }

  return snapshot;
}

class StableTypeWindowCache final {
public:
  std::shared_ptr<const StableTypeWindowSnapshot> SnapshotFor(
      const SuiteBridgeEpgTypeSnapshotRequest &request)
  {
    {
      std::lock_guard<std::mutex> lock(mutex_);
      for (RetainedTypeWindow &window : windows_) {
        if (window.fromTime == request.FromTime() &&
            window.untilTime == request.UntilTime()) {
          window.lastUsed = ++sequence_;
          return window.snapshot;
        }
      }
    }

    const std::shared_ptr<const StableTypeWindowSnapshot> built =
        BuildStableWindowSnapshot(request);

    std::lock_guard<std::mutex> lock(mutex_);
    for (RetainedTypeWindow &window : windows_) {
      if (window.fromTime == request.FromTime() &&
          window.untilTime == request.UntilTime()) {
        window.lastUsed = ++sequence_;
        return window.snapshot;
      }
    }

    if (windows_.size() >= MaximumRetainedWindows) {
      const auto oldest = std::min_element(
          windows_.begin(),
          windows_.end(),
          [](const RetainedTypeWindow &left,
             const RetainedTypeWindow &right) {
            return left.lastUsed < right.lastUsed;
          });
      windows_.erase(oldest);
    }

    RetainedTypeWindow window;
    window.fromTime = request.FromTime();
    window.untilTime = request.UntilTime();
    window.lastUsed = ++sequence_;
    window.snapshot = built;
    windows_.push_back(std::move(window));
    return built;
  }

private:
  std::mutex mutex_;
  std::uint64_t sequence_ = 0;
  std::vector<RetainedTypeWindow> windows_;
};

StableTypeWindowCache &TypeWindowCache()
{
  static StableTypeWindowCache cache;
  return cache;
}

SuiteBridgeEpgMediaType ResolveRealEventMediaType(
    const StableTypeEventIdentity &identity,
    const SuiteBridgeTvScraperAdapter &adapter)
{
  LOCK_CHANNELS_READ;
  LOCK_SCHEDULES_READ;

  const cChannel *channel = Channels->GetByChannelID(identity.channelId);
  if (channel == nullptr) {
    return SuiteBridgeEpgMediaType::None;
  }

  const cSchedule *schedule = Schedules->GetSchedule(channel);
  if (schedule == nullptr) {
    return SuiteBridgeEpgMediaType::None;
  }

  const cEvent *event = schedule->GetEventById(identity.eventId);
  if (event == nullptr ||
      event->StartTime() != identity.startTime ||
      event->EndTime() != identity.endTime) {
    return SuiteBridgeEpgMediaType::None;
  }

  // Match Live: TVScraper receives the real schedule-owned cEvent while the
  // channel and schedule read locks protect its lifetime and relationships.
  return adapter.ResolveMediaType(*event);
}

std::string Usage()
{
  return std::string("Usage: PLUG ") + SuiteBridgePluginIdentity::Name +
      " ETYPES <from-epoch> <until-epoch> <offset> <limit>";
}

SuiteBridgeEpgTypeSnapshotPage CaptureTypeSnapshot(
    const SuiteBridgeEpgTypeSnapshotRequest &request,
    bool &snapshotComplete)
{
  SuiteBridgeEpgTypeSnapshotPage page;
  page.nextOffset = request.Offset();

  const std::shared_ptr<const StableTypeWindowSnapshot> snapshot =
      TypeWindowCache().SnapshotFor(request);
  snapshotComplete = snapshot != nullptr && snapshot->complete;
  if (!snapshotComplete) {
    return page;
  }

  page.done = request.Offset() >= snapshot->events.size();
  if (page.done) {
    return page;
  }

  const SuiteBridgeTvScraperAdapter adapter;
  std::size_t index = static_cast<std::size_t>(request.Offset());
  while (index < snapshot->events.size() && page.scanned < request.Limit()) {
    const StableTypeEventIdentity &identity = snapshot->events[index];
    const SuiteBridgeEpgMediaType mediaType =
        ResolveRealEventMediaType(identity, adapter);

    SuiteBridgeEpgTypeSnapshotPage candidate = page;
    ++candidate.scanned;
    ++candidate.nextOffset;
    candidate.done = candidate.nextOffset >= snapshot->events.size();

    if (mediaType != SuiteBridgeEpgMediaType::None) {
      SuiteBridgeEpgTypeSnapshotItem item;
      item.channelId = identity.channelIdText;
      item.eventId = identity.eventId;
      item.startTime = identity.startTime;
      item.endTime = identity.endTime;
      item.mediaType = mediaType;
      candidate.items.push_back(std::move(item));
    }

    const SuiteBridgeEpgTypeSnapshotPayload bounded(candidate);
    if (!bounded.Complete()) {
      break;
    }

    page = std::move(candidate);
    ++index;
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

  bool snapshotComplete = false;
  const SuiteBridgeEpgTypeSnapshotPage page =
      CaptureTypeSnapshot(request, snapshotComplete);
  if (!snapshotComplete) {
    result.replyCode = 451;
    result.payload = "EPG type snapshot exceeds bounded event capacity";
    return result;
  }

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
