#include "suitebridge_lifecycle.h"

SuiteBridgeLifecycle::SuiteBridgeLifecycle() noexcept
    : state_(SuiteBridgeLifecycleState::Constructed)
{
}

SuiteBridgeLifecycleState SuiteBridgeLifecycle::State() const noexcept
{
  return state_;
}

const char *SuiteBridgeLifecycle::StateName() const noexcept
{
  switch (state_) {
    case SuiteBridgeLifecycleState::Constructed:
      return "constructed";
    case SuiteBridgeLifecycleState::Initialized:
      return "initialized";
    case SuiteBridgeLifecycleState::Started:
      return "started";
    case SuiteBridgeLifecycleState::Stopping:
      return "stopping";
    case SuiteBridgeLifecycleState::Stopped:
      return "stopped";
  }

  return "unknown";
}

bool SuiteBridgeLifecycle::Initialize() noexcept
{
  if (state_ == SuiteBridgeLifecycleState::Initialized ||
      state_ == SuiteBridgeLifecycleState::Started) {
    return true;
  }

  if (state_ != SuiteBridgeLifecycleState::Constructed) {
    return false;
  }

  state_ = SuiteBridgeLifecycleState::Initialized;
  return true;
}

bool SuiteBridgeLifecycle::Start() noexcept
{
  if (state_ == SuiteBridgeLifecycleState::Started) {
    return true;
  }

  if (state_ != SuiteBridgeLifecycleState::Initialized) {
    return false;
  }

  state_ = SuiteBridgeLifecycleState::Started;
  return true;
}

bool SuiteBridgeLifecycle::BeginStop() noexcept
{
  if (state_ == SuiteBridgeLifecycleState::Stopping ||
      state_ == SuiteBridgeLifecycleState::Stopped) {
    return true;
  }

  if (state_ != SuiteBridgeLifecycleState::Initialized &&
      state_ != SuiteBridgeLifecycleState::Started) {
    return false;
  }

  state_ = SuiteBridgeLifecycleState::Stopping;
  return true;
}

bool SuiteBridgeLifecycle::CompleteStop() noexcept
{
  if (state_ == SuiteBridgeLifecycleState::Stopped) {
    return true;
  }

  if (state_ != SuiteBridgeLifecycleState::Stopping) {
    return false;
  }

  state_ = SuiteBridgeLifecycleState::Stopped;
  return true;
}
