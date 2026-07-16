#include "suitebridge_lifecycle.h"

#include <cassert>
#include <cstring>

int main()
{
  SuiteBridgeLifecycle lifecycle;

  assert(lifecycle.State() == SuiteBridgeLifecycleState::Constructed);
  assert(std::strcmp(lifecycle.StateName(), "constructed") == 0);
  assert(!lifecycle.Start());

  assert(lifecycle.Initialize());
  assert(lifecycle.Initialize());
  assert(lifecycle.State() == SuiteBridgeLifecycleState::Initialized);
  assert(std::strcmp(lifecycle.StateName(), "initialized") == 0);

  assert(lifecycle.Start());
  assert(lifecycle.Start());
  assert(lifecycle.State() == SuiteBridgeLifecycleState::Started);
  assert(std::strcmp(lifecycle.StateName(), "started") == 0);

  lifecycle.Stop();
  lifecycle.Stop();
  assert(lifecycle.State() == SuiteBridgeLifecycleState::Stopped);
  assert(std::strcmp(lifecycle.StateName(), "stopped") == 0);

  assert(!lifecycle.Initialize());
  assert(!lifecycle.Start());

  return 0;
}
