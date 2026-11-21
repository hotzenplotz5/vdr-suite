#ifndef VDR_SUITE_BRIDGE_STATUS_MONITOR_H
#define VDR_SUITE_BRIDGE_STATUS_MONITOR_H

#include "suitebridge_osd_state.h"
#include "suitebridge_status_events.h"
#include "suitebridge_status_snapshot.h"

#include <atomic>

#include <vdr/recording.h>
#include <vdr/status.h>

class SuiteBridgeStatusMonitor final : public cStatus {
public:
  SuiteBridgeStatusMonitor() noexcept;

  void Activate() noexcept;
  void Deactivate() noexcept;
  bool IsActive() const noexcept;
  void ObserveRecordingListState() noexcept;

  unsigned long long EventCount(
      SuiteBridgeStatusEventKind kind) const noexcept;
  SuiteBridgeStatusSnapshot CaptureSnapshot() const noexcept;
  SuiteBridgeOsdSnapshot CaptureOsdSnapshot() const noexcept;

protected:
  void ChannelSwitch(
      const cDevice *device,
      int channelNumber,
      bool liveView) override;

  void Recording(
      const cDevice *device,
      const char *name,
      const char *fileName,
      bool on) override;

  void Replaying(
      const cControl *control,
      const char *name,
      const char *fileName,
      bool on) override;

  void TimerChange(
      const cTimer *timer,
      eTimerChange change) override;

  void MarksModified(const cMarks *marks) override;

  void OsdClear(void) override;
  void OsdTitle(const char *title) override;
  void OsdStatusMessage(eMessageType type, const char *message) override;
  void OsdHelpKeys(
      const char *red,
      const char *green,
      const char *yellow,
      const char *blue) override;
  void OsdItem(const char *text, int index, bool selectable) override;
  void OsdCurrentItem(const char *text, int index) override;
  void OsdTextItem(const char *text, bool scroll) override;
  void OsdChannel(const char *text) override;
  void OsdProgramme(
      time_t presentTime,
      const char *presentTitle,
      const char *presentSubtitle,
      time_t followingTime,
      const char *followingTitle,
      const char *followingSubtitle) override;

private:
  void RecordEvent(SuiteBridgeStatusEventKind kind) noexcept;
  void LogSnapshot(const SuiteBridgeStatusSnapshot &snapshot) const noexcept;

  std::atomic<bool> active_;
  SuiteBridgeStatusEvents events_;
  SuiteBridgeOsdState osdState_;
  cStateKey recordingsStateKey_;
};

#endif
