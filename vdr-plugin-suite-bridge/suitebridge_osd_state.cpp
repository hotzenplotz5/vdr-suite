#include "suitebridge_osd_state.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>

namespace {

template <std::size_t Size>
bool CopyBounded(
    std::array<char, Size> &destination,
    const char *source) noexcept
{
  destination.fill('\0');
  if (source == nullptr) {
    return true;
  }

  const std::size_t maximum = Size - 1;
  std::size_t length = 0;
  while (length < maximum && source[length] != '\0') {
    destination[length] = source[length];
    ++length;
  }
  destination[length] = '\0';
  return source[length] == '\0';
}

void CopyEpoch(
    std::array<char, SuiteBridgeOsdSnapshot::EpochHexLength + 1> &destination,
    const char *source) noexcept
{
  destination.fill('\0');
  if (source == nullptr) {
    return;
  }
  for (std::size_t index = 0;
       index < SuiteBridgeOsdSnapshot::EpochHexLength;
       ++index) {
    destination[index] = source[index];
  }
  destination[SuiteBridgeOsdSnapshot::EpochHexLength] = '\0';
}

} // namespace

SuiteBridgeOsdState::SuiteBridgeOsdState() noexcept
    : continuityBroken_(false),
      frameSequence_(0),
      droppedUpdates_(0),
      state_()
{
}

bool SuiteBridgeOsdState::TryLock() const noexcept
{
  return !gate_.test_and_set(std::memory_order_acquire);
}

void SuiteBridgeOsdState::Unlock() const noexcept
{
  gate_.clear(std::memory_order_release);
}

void SuiteBridgeOsdState::MarkDroppedUpdate() noexcept
{
  (void)droppedUpdates_.Increment();
  continuityBroken_.store(true, std::memory_order_release);
}

void SuiteBridgeOsdState::ResetPayloadLocked() noexcept
{
  const unsigned long long sequence = state_.frameSequence;
  const unsigned long long dropped = droppedUpdates_.Value();
  state_ = SuiteBridgeOsdSnapshot{};
  state_.frameSequence = sequence;
  state_.droppedUpdates = dropped;
}

void SuiteBridgeOsdState::StartSurfaceLocked(
    SuiteBridgeOsdFrameKind kind) noexcept
{
  const bool sameFamily =
      state_.active && state_.kind == kind;
  if (sameFamily) {
    return;
  }

  ResetPayloadLocked();
  const SuiteBridgeCounterEpoch epoch;
  CopyEpoch(state_.osdEpoch, epoch.Data());
  state_.active = true;
  state_.complete = true;
  state_.consistent = true;
  state_.kind = kind;
  continuityBroken_.store(false, std::memory_order_release);
}

void SuiteBridgeOsdState::StartMenuLocked() noexcept
{
  StartSurfaceLocked(SuiteBridgeOsdFrameKind::Menu);
}

void SuiteBridgeOsdState::StartChannelInfoLocked() noexcept
{
  StartSurfaceLocked(SuiteBridgeOsdFrameKind::ChannelInfo);
}

void SuiteBridgeOsdState::AdvanceLocked() noexcept
{
  state_.frameSequence = frameSequence_.Increment();
  state_.observedAtMilliseconds = static_cast<unsigned long long>(
      std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::system_clock::now().time_since_epoch()).count());
  state_.droppedUpdates = droppedUpdates_.Value();
  if (continuityBroken_.load(std::memory_order_acquire) ||
      frameSequence_.Overflowed()) {
    state_.complete = false;
  }
}

void SuiteBridgeOsdState::Clear() noexcept
{
  if (!TryLock()) {
    MarkDroppedUpdate();
    return;
  }

  ResetPayloadLocked();
  state_.active = false;
  state_.complete = true;
  state_.consistent = true;
  state_.kind = SuiteBridgeOsdFrameKind::Inactive;
  continuityBroken_.store(false, std::memory_order_release);
  AdvanceLocked();
  Unlock();
}

void SuiteBridgeOsdState::Title(const char *text) noexcept
{
  if (!TryLock()) {
    MarkDroppedUpdate();
    return;
  }
  StartMenuLocked();
  if (!CopyBounded(state_.title, text)) state_.complete = false;
  AdvanceLocked();
  Unlock();
}

void SuiteBridgeOsdState::StatusMessage(const char *text) noexcept
{
  if (!TryLock()) {
    MarkDroppedUpdate();
    return;
  }
  StartMenuLocked();
  if (!CopyBounded(state_.statusMessage, text)) state_.complete = false;
  AdvanceLocked();
  Unlock();
}

void SuiteBridgeOsdState::HelpKeys(
    const char *red,
    const char *green,
    const char *yellow,
    const char *blue) noexcept
{
  if (!TryLock()) {
    MarkDroppedUpdate();
    return;
  }
  StartMenuLocked();
  if (!CopyBounded(state_.red, red)) state_.complete = false;
  if (!CopyBounded(state_.green, green)) state_.complete = false;
  if (!CopyBounded(state_.yellow, yellow)) state_.complete = false;
  if (!CopyBounded(state_.blue, blue)) state_.complete = false;
  AdvanceLocked();
  Unlock();
}

void SuiteBridgeOsdState::Item(
    const char *text,
    int index,
    bool selectable) noexcept
{
  if (!TryLock()) {
    MarkDroppedUpdate();
    return;
  }
  StartMenuLocked();
  if (index < 0 ||
      static_cast<std::size_t>(index) >= SuiteBridgeOsdSnapshot::MaximumItems) {
    state_.complete = false;
    AdvanceLocked();
    Unlock();
    return;
  }

  const std::size_t itemIndex = static_cast<std::size_t>(index);
  if (!CopyBounded(state_.items[itemIndex].text, text)) state_.complete = false;
  state_.items[itemIndex].selectable = selectable;
  state_.itemCount = std::max(state_.itemCount, itemIndex + 1);
  AdvanceLocked();
  Unlock();
}

void SuiteBridgeOsdState::CurrentItem(
    const char *text,
    int index) noexcept
{
  if (!TryLock()) {
    MarkDroppedUpdate();
    return;
  }
  StartMenuLocked();
  if (index < 0 ||
      static_cast<std::size_t>(index) >= SuiteBridgeOsdSnapshot::MaximumItems) {
    state_.selectedIndex = -1;
    state_.complete = false;
    AdvanceLocked();
    Unlock();
    return;
  }

  const std::size_t itemIndex = static_cast<std::size_t>(index);
  state_.selectedIndex = index;
  state_.itemCount = std::max(state_.itemCount, itemIndex + 1);
  if (text != nullptr &&
      !CopyBounded(state_.items[itemIndex].text, text)) {
    state_.complete = false;
  }
  AdvanceLocked();
  Unlock();
}

void SuiteBridgeOsdState::TextItem(
    const char *text,
    bool scroll) noexcept
{
  (void)scroll;
  if (!TryLock()) {
    MarkDroppedUpdate();
    return;
  }
  StartMenuLocked();
  if (text == nullptr) {
    state_.complete = false;
  } else if (!CopyBounded(state_.detailText, text)) {
    state_.complete = false;
  }
  AdvanceLocked();
  Unlock();
}

void SuiteBridgeOsdState::Channel(const char *text) noexcept
{
  if (!TryLock()) {
    MarkDroppedUpdate();
    return;
  }
  StartChannelInfoLocked();
  if (!CopyBounded(state_.channel, text)) state_.complete = false;
  AdvanceLocked();
  Unlock();
}

void SuiteBridgeOsdState::Programme(
    std::int64_t presentTime,
    const char *presentTitle,
    const char *presentSubtitle,
    std::int64_t followingTime,
    const char *followingTitle,
    const char *followingSubtitle) noexcept
{
  if (!TryLock()) {
    MarkDroppedUpdate();
    return;
  }
  StartChannelInfoLocked();
  state_.presentTime = presentTime;
  state_.followingTime = followingTime;
  if (!CopyBounded(state_.presentTitle, presentTitle)) state_.complete = false;
  if (!CopyBounded(state_.presentSubtitle, presentSubtitle)) state_.complete = false;
  if (!CopyBounded(state_.followingTitle, followingTitle)) state_.complete = false;
  if (!CopyBounded(state_.followingSubtitle, followingSubtitle)) state_.complete = false;
  AdvanceLocked();
  Unlock();
}

SuiteBridgeOsdSnapshot SuiteBridgeOsdState::CaptureSnapshot() const noexcept
{
  if (!TryLock()) {
    SuiteBridgeOsdSnapshot unavailable;
    unavailable.consistent = false;
    unavailable.complete = false;
    unavailable.droppedUpdates = droppedUpdates_.Value();
    return unavailable;
  }

  SuiteBridgeOsdSnapshot snapshot = state_;
  Unlock();

  snapshot.droppedUpdates = droppedUpdates_.Value();
  if (continuityBroken_.load(std::memory_order_acquire)) {
    snapshot.complete = false;
  }
  return snapshot;
}

unsigned long long SuiteBridgeOsdState::DroppedUpdates() const noexcept
{
  return droppedUpdates_.Value();
}
