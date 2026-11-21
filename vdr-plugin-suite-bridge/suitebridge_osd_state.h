#ifndef VDR_SUITE_BRIDGE_OSD_STATE_H
#define VDR_SUITE_BRIDGE_OSD_STATE_H

#include "suitebridge_counter_continuity.h"
#include "suitebridge_osd_snapshot.h"

#include <atomic>
#include <cstdint>

class SuiteBridgeOsdState final {
public:
  SuiteBridgeOsdState() noexcept;

  void Clear() noexcept;
  void Title(const char *text) noexcept;
  void StatusMessage(const char *text) noexcept;
  void HelpKeys(
      const char *red,
      const char *green,
      const char *yellow,
      const char *blue) noexcept;
  void Item(const char *text, int index, bool selectable) noexcept;
  void CurrentItem(const char *text, int index) noexcept;
  void TextItem(const char *text, bool scroll) noexcept;
  void Channel(const char *text) noexcept;
  void Programme(
      std::int64_t presentTime,
      const char *presentTitle,
      const char *presentSubtitle,
      std::int64_t followingTime,
      const char *followingTitle,
      const char *followingSubtitle) noexcept;

  SuiteBridgeOsdSnapshot CaptureSnapshot() const noexcept;
  unsigned long long DroppedUpdates() const noexcept;

private:
  bool TryLock() const noexcept;
  void Unlock() const noexcept;
  void MarkDroppedUpdate() noexcept;
  void ResetPayloadLocked() noexcept;
  void StartMenuLocked() noexcept;
  void StartChannelInfoLocked() noexcept;
  void StartSurfaceLocked(SuiteBridgeOsdFrameKind kind) noexcept;
  void AdvanceLocked() noexcept;

  mutable std::atomic_flag gate_ = ATOMIC_FLAG_INIT;
  std::atomic<bool> continuityBroken_;
  SuiteBridgeSaturatingCounter frameSequence_;
  SuiteBridgeSaturatingCounter droppedUpdates_;
  SuiteBridgeOsdSnapshot state_;
};

#endif
