#include "suitebridge_lifecycle.h"

#include <cassert>
#include <cstring>

int main()
{
  {
    SuiteBridgeLifecycle lifecycle;

    assert(lifecycle.State() == SuiteBridgeLifecycleState::Constructed);
    assert(std::strcmp(lifecycle.StateName(), "constructed") == 0);
    assert(!lifecycle.Start());
    assert(!lifecycle.BeginStop());
    assert(!lifecycle.CompleteStop());

    assert(lifecycle.Initialize());
    assert(lifecycle.Initialize());
    assert(lifecycle.State() == SuiteBridgeLifecycleState::Initialized);
    assert(std::strcmp(lifecycle.StateName(), "initialized") == 0);

    assert(lifecycle.Start());
    assert(lifecycle.Start());
    assert(lifecycle.State() == SuiteBridgeLifecycleState::Started);
    assert(std::strcmp(lifecycle.StateName(), "started") == 0);

    assert(lifecycle.BeginStop());
    assert(lifecycle.BeginStop());
    assert(lifecycle.State() == SuiteBridgeLifecycleState::Stopping);
    assert(std::strcmp(lifecycle.StateName(), "stopping") == 0);

    assert(!lifecycle.Initialize());
    assert(!lifecycle.Start());

    assert(lifecycle.CompleteStop());
    assert(lifecycle.CompleteStop());
    assert(lifecycle.State() == SuiteBridgeLifecycleState::Stopped);
    assert(std::strcmp(lifecycle.StateName(), "stopped") == 0);

    assert(lifecycle.BeginStop());
    assert(!lifecycle.Initialize());
    assert(!lifecycle.Start());
  }

  {
    SuiteBridgeLifecycle lifecycle;

    assert(lifecycle.Initialize());
    assert(lifecycle.BeginStop());
    assert(lifecycle.State() == SuiteBridgeLifecycleState::Stopping);
    assert(lifecycle.CompleteStop());
    assert(lifecycle.State() == SuiteBridgeLifecycleState::Stopped);
  }

  return 0;
}
