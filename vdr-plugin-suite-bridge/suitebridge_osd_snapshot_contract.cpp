#include "suitebridge_osd_snapshot_contract.h"

#include <cstdio>
#include <string>

namespace {

const char *KindName(SuiteBridgeOsdFrameKind kind) noexcept
{
  switch (kind) {
    case SuiteBridgeOsdFrameKind::Inactive: return "inactive";
    case SuiteBridgeOsdFrameKind::Menu: return "menu";
    case SuiteBridgeOsdFrameKind::ChannelInfo: return "channel_info";
  }
  return "inactive";
}

} // namespace

SuiteBridgeOsdSnapshotPayload::SuiteBridgeOsdSnapshotPayload(
    const SuiteBridgeOsdSnapshot &snapshot)
{
  data_.reserve(8192);

  if (snapshot.itemCount > SuiteBridgeOsdSnapshot::MaximumItems) {
    complete_ = false;
    return;
  }

  Append("{\"osd_schema\":");
  AppendUnsigned(SchemaVersion());
  Append(",\"active\":");
  Append(snapshot.active ? "true" : "false");
  Append(",\"complete\":");
  Append(snapshot.complete ? "true" : "false");
  Append(",\"consistent\":");
  Append(snapshot.consistent ? "true" : "false");
  Append(",\"kind\":");
  AppendJsonString(KindName(snapshot.kind));
  Append(",\"frame_sequence\":");
  AppendUnsigned(snapshot.frameSequence);
  Append(",\"observed_at_ms\":");
  AppendUnsigned(snapshot.observedAtMilliseconds);
  Append(",\"dropped_updates\":");
  AppendUnsigned(snapshot.droppedUpdates);
  Append(",\"osd_epoch\":");
  AppendJsonString(snapshot.osdEpoch.data());
  Append(",\"title\":");
  AppendJsonString(snapshot.title.data());
  Append(",\"status\":");
  AppendJsonString(snapshot.statusMessage.data());
  Append(",\"red\":");
  AppendJsonString(snapshot.red.data());
  Append(",\"green\":");
  AppendJsonString(snapshot.green.data());
  Append(",\"yellow\":");
  AppendJsonString(snapshot.yellow.data());
  Append(",\"blue\":");
  AppendJsonString(snapshot.blue.data());
  Append(",\"items\":[");

  for (std::size_t index = 0; index < snapshot.itemCount; ++index) {
    if (index != 0) Append(",");
    Append("{\"text\":");
    AppendJsonString(snapshot.items[index].text.data());
    Append(",\"selectable\":");
    Append(snapshot.items[index].selectable ? "true" : "false");
    Append("}");
  }

  Append("],\"selected_index\":");
  AppendSigned(snapshot.selectedIndex);
  Append(",\"text\":");
  AppendJsonString(snapshot.detailText.data());
  Append(",\"channel\":");
  AppendJsonString(snapshot.channel.data());
  Append(",\"programme\":{\"present_time\":");
  AppendSigned(snapshot.presentTime);
  Append(",\"present_title\":");
  AppendJsonString(snapshot.presentTitle.data());
  Append(",\"present_subtitle\":");
  AppendJsonString(snapshot.presentSubtitle.data());
  Append(",\"following_time\":");
  AppendSigned(snapshot.followingTime);
  Append(",\"following_title\":");
  AppendJsonString(snapshot.followingTitle.data());
  Append(",\"following_subtitle\":");
  AppendJsonString(snapshot.followingSubtitle.data());
  Append("}}");

  if (!complete_) data_.clear();
}

bool SuiteBridgeOsdSnapshotPayload::Append(const std::string &value)
{
  if (!complete_ || data_.size() > MaximumPayloadBytes ||
      value.size() > MaximumPayloadBytes - data_.size()) {
    complete_ = false;
    return false;
  }
  data_ += value;
  return true;
}

bool SuiteBridgeOsdSnapshotPayload::AppendJsonString(const char *value)
{
  if (!Append("\"")) return false;
  if (value != nullptr) {
    for (const unsigned char *cursor =
             reinterpret_cast<const unsigned char *>(value);
         *cursor != '\0';
         ++cursor) {
      const unsigned char character = *cursor;
      switch (character) {
        case '"': if (!Append("\\\"")) return false; break;
        case '\\': if (!Append("\\\\")) return false; break;
        case '\b': if (!Append("\\b")) return false; break;
        case '\f': if (!Append("\\f")) return false; break;
        case '\n': if (!Append("\\n")) return false; break;
        case '\r': if (!Append("\\r")) return false; break;
        case '\t': if (!Append("\\t")) return false; break;
        default:
          if (character < 0x20) {
            char escaped[7] = {0};
            std::snprintf(
                escaped,
                sizeof(escaped),
                "\\u%04x",
                static_cast<unsigned int>(character));
            if (!Append(escaped)) return false;
          } else {
            if (!Append(std::string(1, static_cast<char>(character)))) {
              return false;
            }
          }
      }
    }
  }
  return Append("\"");
}

bool SuiteBridgeOsdSnapshotPayload::AppendUnsigned(
    unsigned long long value)
{
  return Append(std::to_string(value));
}

bool SuiteBridgeOsdSnapshotPayload::AppendSigned(long long value)
{
  return Append(std::to_string(value));
}

const char *SuiteBridgeOsdSnapshotPayload::Data() const noexcept
{
  return data_.c_str();
}

std::size_t SuiteBridgeOsdSnapshotPayload::Size() const noexcept
{
  return data_.size();
}

bool SuiteBridgeOsdSnapshotPayload::Complete() const noexcept
{
  return complete_;
}
