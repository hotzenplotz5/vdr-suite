#include "suitebridge_osd_snapshot_contract.h"

#include <cassert>
#include <cstring>
#include <string>

namespace {

bool Contains(const SuiteBridgeOsdSnapshotPayload &payload, const char *needle)
{
  return std::string(payload.Data()).find(needle) != std::string::npos;
}

} // namespace

int main()
{
  static_assert(SuiteBridgeOsdSnapshotPayload::SchemaVersion() == 1);
  static_assert(
      SuiteBridgeOsdSnapshotPayload::MaximumPayloadBytes <= 131072);

  SuiteBridgeOsdSnapshot frame;
  frame.active = true;
  frame.complete = true;
  frame.consistent = true;
  frame.kind = SuiteBridgeOsdFrameKind::Menu;
  frame.frameSequence = 42;
  frame.observedAtMilliseconds = 123456;
  frame.droppedUpdates = 0;
  std::strcpy(frame.osdEpoch.data(), "0123456789abcdef0123456789abcdef");
  std::strcpy(frame.title.data(), "Main \"menu\"");
  std::strcpy(frame.statusMessage.data(), "Ready\nNow");
  frame.itemCount = 2;
  std::strcpy(frame.items[0].text.data(), "Recordings");
  frame.items[0].selectable = true;
  std::strcpy(frame.items[1].text.data(), "Separator");
  frame.items[1].selectable = false;
  frame.selectedIndex = 0;

  const SuiteBridgeOsdSnapshotPayload payload(frame);
  assert(payload.Complete());
  assert(payload.Size() > 0);
  assert(payload.Size() <= SuiteBridgeOsdSnapshotPayload::MaximumPayloadBytes);
  assert(Contains(payload, "\"osd_schema\":1"));
  assert(Contains(payload, "\"active\":true"));
  assert(Contains(payload, "\"kind\":\"menu\""));
  assert(Contains(payload, "\"frame_sequence\":42"));
  assert(Contains(
      payload,
      "\"osd_epoch\":\"0123456789abcdef0123456789abcdef\""));
  assert(Contains(payload, "\"title\":\"Main \\\"menu\\\"\""));
  assert(Contains(payload, "\"status\":\"Ready\\nNow\""));
  assert(Contains(
      payload,
      "{\"text\":\"Recordings\",\"selectable\":true}"));
  assert(Contains(
      payload,
      "{\"text\":\"Separator\",\"selectable\":false}"));
  assert(Contains(payload, "\"selected_index\":0"));

  SuiteBridgeOsdSnapshot inactive;
  const SuiteBridgeOsdSnapshotPayload inactivePayload(inactive);
  assert(inactivePayload.Complete());
  assert(Contains(inactivePayload, "\"active\":false"));
  assert(Contains(inactivePayload, "\"kind\":\"inactive\""));
  assert(Contains(inactivePayload, "\"items\":[]"));

  SuiteBridgeOsdSnapshot oversized;
  oversized.itemCount = SuiteBridgeOsdSnapshot::MaximumItems;
  for (auto &item : oversized.items) {
    for (std::size_t index = 0;
         index < SuiteBridgeOsdItemSnapshot::TextBytes;
         ++index) {
      item.text[index] = '\n';
    }
  }
  const SuiteBridgeOsdSnapshotPayload oversizedPayload(oversized);
  assert(!oversizedPayload.Complete());
  assert(oversizedPayload.Size() == 0);

  return 0;
}
