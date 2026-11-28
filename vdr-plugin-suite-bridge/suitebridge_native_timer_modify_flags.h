#ifndef VDR_SUITE_BRIDGE_NATIVE_TIMER_MODIFY_FLAGS_H
#define VDR_SUITE_BRIDGE_NATIVE_TIMER_MODIFY_FLAGS_H

// Update only the two explicitly editable VDR Timer flags. Preserve
// Instant/Spawned/Avoid and all other unrelated native bits.
namespace vdrsuite_suitebridge {
inline unsigned updatedNativeTimerFlags(
    unsigned existing, unsigned activeMask, unsigned vpsMask,
    bool enabled, bool vps)
{
  const unsigned retained = existing & ~(activeMask | vpsMask);
  return retained | (enabled ? activeMask : 0U) | (vps ? vpsMask : 0U);
}
}

#endif
