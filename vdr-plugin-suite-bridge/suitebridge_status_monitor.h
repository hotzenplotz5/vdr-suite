#ifndef VDR_SUITE_BRIDGE_STATUS_MONITOR_H
#define VDR_SUITE_BRIDGE_STATUS_MONITOR_H

#include "suitebridge_status_events.h"

#include <atomic>

#include <vdr/status.h>

class SuiteBridgeStatusMonitor final : public cStatus {
public:
  SuiteBridgeStatusMonitor() noexcept;

  void Activate() noexcept;
  void Deactivate() noexcept;
  bool IsActive() const noexcept;

  unsigned long long EventCount(
      SuiteBridgeStatusEventKind kind) const noexcept;

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

private:
  std::atomic<bool> active_;
  SuiteBridgeStatusEvents events_;
};

#endif
