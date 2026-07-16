#ifndef VDR_SUITE_BRIDGE_LIFECYCLE_H
#define VDR_SUITE_BRIDGE_LIFECYCLE_H

enum class SuiteBridgeLifecycleState {
  Constructed,
  Initialized,
  Started,
  Stopped,
};

class SuiteBridgeLifecycle final {
public:
  SuiteBridgeLifecycle() noexcept;

  SuiteBridgeLifecycleState State() const noexcept;
  const char *StateName() const noexcept;

  bool Initialize() noexcept;
  bool Start() noexcept;
  void Stop() noexcept;

private:
  SuiteBridgeLifecycleState state_;
};

#endif
