#include "suitebridge_local_contract.h"
#include "suitebridge_status_snapshot.h"

#include <cassert>
#include <cstring>
#include <type_traits>

int main()
{
  static_assert(SuiteBridgeLocalContractPayload::SchemaVersion() == 1);
  static_assert(SuiteBridgeLocalContractPayload::Capacity() == 320);
  static_assert(
      std::is_copy_constructible<SuiteBridgeLocalContractPayload>::value);
  static_assert(
      !std::is_copy_assignable<SuiteBridgeLocalContractPayload>::value);

  const SuiteBridgeStatusSnapshot activeSnapshot(
      true,
      12,
      2,
      3,
      4);
  const SuiteBridgeLocalContractPayload activePayload(
      1,
      activeSnapshot);

  const char *expectedActive =
      "{\"contract_schema\":1,\"capability_schema\":1,\"snapshot_schema\":1,\"active\":true,\"total\":21,\"channel_switch\":12,\"recording\":2,\"replaying\":3,\"timer_change\":4}";

  assert(activePayload.Complete());
  assert(activePayload.Size() == std::strlen(expectedActive));
  assert(activePayload.Size() < activePayload.Capacity());
  assert(std::strcmp(activePayload.Data(), expectedActive) == 0);

  const SuiteBridgeStatusSnapshot inactiveSnapshot(
      false,
      0,
      0,
      0,
      0);
  const SuiteBridgeLocalContractPayload inactivePayload(
      7,
      inactiveSnapshot);

  const char *expectedInactive =
      "{\"contract_schema\":1,\"capability_schema\":7,\"snapshot_schema\":1,\"active\":false,\"total\":0,\"channel_switch\":0,\"recording\":0,\"replaying\":0,\"timer_change\":0}";

  assert(inactivePayload.Complete());
  assert(inactivePayload.Size() == std::strlen(expectedInactive));
  assert(std::strcmp(inactivePayload.Data(), expectedInactive) == 0);

  return 0;
}
