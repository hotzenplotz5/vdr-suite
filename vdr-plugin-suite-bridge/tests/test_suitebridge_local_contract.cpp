#include "suitebridge_local_contract.h"
#include "suitebridge_status_snapshot.h"

#include <cassert>
#include <cstring>
#include <type_traits>

int main()
{
  static_assert(SuiteBridgeLocalContractPayload::SchemaVersion() == 2);
  static_assert(SuiteBridgeLocalContractPayload::Capacity() == 448);
  static_assert(
      std::is_copy_constructible<SuiteBridgeLocalContractPayload>::value);
  static_assert(
      !std::is_copy_assignable<SuiteBridgeLocalContractPayload>::value);

  const char *activeEpoch = "0123456789abcdef0123456789abcdef";
  const SuiteBridgeStatusSnapshot activeSnapshot(
      true,
      12,
      2,
      3,
      4,
      activeEpoch,
      false);
  const SuiteBridgeLocalContractPayload activePayload(
      1,
      activeSnapshot);
  const char *expectedActive =
      "{\"contract_schema\":2,\"capability_schema\":1,\"snapshot_schema\":2,\"active\":true,\"total\":21,\"channel_switch\":12,\"recording\":2,\"replaying\":3,\"timer_change\":4,\"counter_epoch\":\"0123456789abcdef0123456789abcdef\",\"counter_overflow\":false}";

  assert(activePayload.Complete());
  assert(activePayload.Size() == std::strlen(expectedActive));
  assert(activePayload.Size() < activePayload.Capacity());
  assert(std::strcmp(activePayload.Data(), expectedActive) == 0);

  const char *inactiveEpoch = "fedcba9876543210fedcba9876543210";
  const SuiteBridgeStatusSnapshot inactiveSnapshot(
      false,
      0,
      0,
      0,
      0,
      inactiveEpoch,
      true);
  const SuiteBridgeLocalContractPayload inactivePayload(
      7,
      inactiveSnapshot);
  const char *expectedInactive =
      "{\"contract_schema\":2,\"capability_schema\":7,\"snapshot_schema\":2,\"active\":false,\"total\":0,\"channel_switch\":0,\"recording\":0,\"replaying\":0,\"timer_change\":0,\"counter_epoch\":\"fedcba9876543210fedcba9876543210\",\"counter_overflow\":true}";

  assert(inactivePayload.Complete());
  assert(inactivePayload.Size() == std::strlen(expectedInactive));
  assert(std::strcmp(inactivePayload.Data(), expectedInactive) == 0);

  return 0;
}
