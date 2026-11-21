#include "suitebridge_osd_state.h"

#include <cassert>
#include <cstring>
#include <string>

namespace {

bool LowerHexEpoch(const char *value)
{
  if (value == nullptr || std::strlen(value) != 32) return false;
  for (std::size_t index = 0; index < 32; ++index) {
    const char c = value[index];
    if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'))) return false;
  }
  return true;
}

}

int main()
{
  static_assert(SuiteBridgeOsdSnapshot::SchemaVersion() == 1);
  static_assert(SuiteBridgeOsdSnapshot::MaximumItems == 128);

  SuiteBridgeOsdState state;
  SuiteBridgeOsdSnapshot frame = state.CaptureSnapshot();
  assert(frame.consistent);
  assert(!frame.active);
  assert(frame.kind == SuiteBridgeOsdFrameKind::Inactive);
  assert(frame.frameSequence == 0);

  state.Title("Main menu");
  state.Item("Recordings", 0, true);
  state.Item("Setup", 1, true);
  state.CurrentItem("Recordings", 0);
  state.HelpKeys("Red", "Green", "Yellow", "Blue");
  state.StatusMessage("Ready");

  frame = state.CaptureSnapshot();
  assert(frame.consistent);
  assert(frame.active);
  assert(frame.complete);
  assert(frame.kind == SuiteBridgeOsdFrameKind::Menu);
  assert(LowerHexEpoch(frame.osdEpoch.data()));
  assert(frame.frameSequence == 6);
  assert(frame.observedAtMilliseconds > 0);
  assert(std::strcmp(frame.title.data(), "Main menu") == 0);
  assert(frame.itemCount == 2);
  assert(std::strcmp(frame.items[0].text.data(), "Recordings") == 0);
  assert(frame.items[0].selectable);
  assert(frame.selectedIndex == 0);
  assert(std::strcmp(frame.statusMessage.data(), "Ready") == 0);
  const std::string firstEpoch(frame.osdEpoch.data());

  state.TextItem(nullptr, true);
  frame = state.CaptureSnapshot();
  assert(!frame.complete);
  assert(frame.frameSequence == 7);

  state.Clear();
  frame = state.CaptureSnapshot();
  assert(!frame.active);
  assert(frame.complete);
  assert(frame.kind == SuiteBridgeOsdFrameKind::Inactive);
  assert(frame.frameSequence == 8);

  state.Channel("1 Das Erste HD");
  state.Programme(
      1000,
      "Present",
      "Episode",
      2000,
      "Following",
      "Next episode");
  frame = state.CaptureSnapshot();
  assert(frame.active);
  assert(frame.complete);
  assert(frame.kind == SuiteBridgeOsdFrameKind::ChannelInfo);
  assert(frame.frameSequence == 10);
  assert(LowerHexEpoch(frame.osdEpoch.data()));
  assert(firstEpoch != frame.osdEpoch.data());
  assert(std::strcmp(frame.channel.data(), "1 Das Erste HD") == 0);
  assert(frame.presentTime == 1000);
  assert(std::strcmp(frame.presentTitle.data(), "Present") == 0);
  assert(frame.followingTime == 2000);

  std::string oversized(SuiteBridgeOsdSnapshot::TitleBytes + 20, 'x');
  state.Clear();
  state.Title(oversized.c_str());
  frame = state.CaptureSnapshot();
  assert(frame.active);
  assert(!frame.complete);
  assert(std::strlen(frame.title.data()) == SuiteBridgeOsdSnapshot::TitleBytes);

  state.Item("out of range", 500, true);
  frame = state.CaptureSnapshot();
  assert(!frame.complete);
  assert(frame.itemCount == 0);
  assert(state.DroppedUpdates() == 0);

  return 0;
}
